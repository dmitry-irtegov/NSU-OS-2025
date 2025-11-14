package process

import (
	"errors"
	"fmt"
	"os"
	"os/signal"
	"shell/internal/prompt"
	"sync"
	"syscall"
	"unsafe"
)

const (
	Background int = iota
	Foreground
	Stopped
)

type ProcessData struct {
	Status       int
	jobID        int
	IsPipe       bool
	TextCommands string
}

type pgid = int
type Manager struct {
	processes map[pgid]*ProcessData
	mu        sync.Mutex
	Prompt    prompt.Prompt
	errChan   chan<- error
	jobsCount int
	jobsID    int
}

func NewProcessManager(prompt prompt.Prompt, errChan chan<- error) *Manager {
	pm := &Manager{
		processes: make(map[pgid]*ProcessData),
		Prompt:    prompt,
		errChan:   errChan,
	}
	go pm.handleStopSignals()
	go pm.handleInterruptSignals()
	return pm
}

// only with mu lock
func (pm *Manager) handleKillerror(err error) error {
	if err != nil {
		if !errors.Is(err, syscall.ESRCH) {
			return err
		}
	}
	return nil
}

// only with mu lock
func (pm *Manager) interruptFGroup(pid pgid) error {
	if data := pm.processes[pid]; data.Status == Foreground {

		err := syscall.Kill(pid, syscall.SIGINT)
		if pm.handleKillerror(err) != nil {
			return fmt.Errorf("не удалось прервать процесс %d по причине: %w", pid, err)
		}
	}
	return nil
}

// only with mu lock
func (pm *Manager) toBackgroundGroup(pid pgid) error {
	if data := pm.processes[pid]; data.Status == Foreground {
		err := syscall.Kill(pid, syscall.SIGSTOP)
		if pm.handleKillerror(err) != nil {
			return fmt.Errorf("не удалось остановить процесс %d по причине: %w", pid, err)
		}
	}
	return nil
}

func (pm *Manager) handleInterruptSignals() {
	c := make(chan os.Signal, 1)
	signal.Notify(c, syscall.SIGINT, syscall.SIGQUIT)
	for sig := range c {
		if sig == syscall.SIGQUIT {
			continue
		}
		pm.mu.Lock()
		for pid := range pm.processes {
			err := pm.interruptFGroup(pid)
			if err != nil {
				pm.errChan <- err
			}
		}
		pm.mu.Unlock()
	}
}

func (pm *Manager) handleStopSignals() {
	c := make(chan os.Signal, 1)
	signal.Notify(c, syscall.SIGTSTP)
	for _ = range c {
		pm.mu.Lock()
		for pid := range pm.processes {
			err := pm.toBackgroundGroup(pid)
			if err != nil {
				pm.errChan <- err
			}
		}
		pm.mu.Unlock()
	}
}

func (pm *Manager) KillAll() {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	for pid := range pm.processes {
		_ = syscall.Kill(pid, syscall.SIGKILL)
	}
}

func (pm *Manager) Wait(pid int, data ProcessData) {
	signal.Ignore(syscall.SIGTTOU, syscall.SIGTTIN)
	defer signal.Reset(syscall.SIGTTOU, syscall.SIGTTIN)
	pm.mu.Lock()
	pm.processes[pid] = &data
	if data.Status == Background {
		pm.jobsCount++
		pm.jobsID++
		data.jobID = pm.jobsID
		fmt.Printf("\n[%d] %d\n", data.jobID, pid)
	}
	pm.mu.Unlock()
	if (data.Status == Foreground) && (!data.IsPipe) {
		errno := pm.tcsetpgrp(pid)
		if (errno != 0) && (errno.Error() != "no such process") {
			pm.errChan <- fmt.Errorf("ошибка передачи управления процессу: %w", errno)
			return
		}
	}

	var ws syscall.WaitStatus
	_, err := syscall.Wait4(pid, &ws, syscall.WUNTRACED, nil)
	if (err != nil) && (err.Error() != "no child processes") {
		fmt.Println("Ошибка ожидания процесса: ", err)
	}
	shellPgid := os.Getpid()
	errno := pm.tcsetpgrp(shellPgid)
	if errno != 0 {
		pm.errChan <- fmt.Errorf("ошибка возврата управления процессу: %w", errno)
		return
	}
	if ws.Stopped() {
		if data.Status == Foreground {
			pm.stop(pid)
		}
	} else {
		pm.delete(pid)
	}
}

func (pm *Manager) tcsetpgrp(pid pgid) syscall.Errno {
	_, _, errno := syscall.Syscall(syscall.SYS_IOCTL, uintptr(syscall.Stdin),
		uintptr(syscall.TIOCSPGRP), uintptr(unsafe.Pointer(&pid)))
	return errno
}

func (pm *Manager) stop(pid pgid) {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	data := pm.processes[pid]
	data.Status = Stopped
	pm.jobsCount++
	pm.jobsID++
	data.jobID = pm.jobsID
	fmt.Printf("\n[%d]+ Остановлен     %s\n", data.jobID, data.TextCommands)
	pm.processes[pid] = data
}

func (pm *Manager) delete(pid pgid) {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	data, ok := pm.processes[pid]
	if !ok {
		return
	}
	if data.Status != Foreground {
		fmt.Printf("\n[%d]+ Завершен     %s\n", data.jobID, data.TextCommands)
		pm.jobsCount--
		if pm.jobsCount == 0 {
			pm.jobsID = 0
		}
		pm.Prompt.PrintPrompt()
	}
	delete(pm.processes, pid)
}

func (pm *Manager) GetJobs() []string {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	jobs := make([]string, 0, 32)
	for _, data := range pm.processes {
		switch data.Status {
		case Background:
			jobs = append(jobs, fmt.Sprintf("[%d] Выполняется     %s", data.jobID, data.TextCommands))
		case Stopped:
			jobs = append(jobs, fmt.Sprintf("[%d] Остановлен     %s", data.jobID, data.TextCommands))
		case Foreground:
		}
	}
	return jobs
}

func (pm *Manager) GetNearestJobID(typeReq int) (int, error) {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	nearestID := -1
	for _, data := range pm.processes {
		if data.jobID > nearestID {
			switch typeReq {
			case Stopped:
			case Foreground:
				if data.Status != Foreground {
					nearestID = data.jobID
				}
			case Background:
				if data.Status == Stopped {
					nearestID = data.jobID
				}
			default:
				return -1, fmt.Errorf("неизвестный тип запроса")
			}
		}
	}
	if nearestID == -1 {
		return -1, fmt.Errorf("нет доступных заданий")
	}
	return nearestID, nil
}

func (pm *Manager) ToBackground(jobID int) error {
	pm.mu.Lock()
	defer pm.mu.Unlock()
	for pid, data := range pm.processes {
		if data.jobID == jobID {
			if data.Status == Stopped {
				err := syscall.Kill(pid, syscall.SIGCONT)
				if err != nil {
					return err
				}
			} else {
				return fmt.Errorf("job [%d] уже выполняется в фоне", jobID)
			}
			pm.jobsID--
			pm.jobsCount--
			data.Status = Background
			fmt.Printf("[%d] %d\n", data.jobID, pid)
			go pm.Wait(pid, *data)
			return nil
		}
	}
	return fmt.Errorf("job %d не найден", jobID)
}

func (pm *Manager) ToForeground(jobID int) error {
	pm.mu.Lock()
	for pid, data := range pm.processes {
		if data.jobID == jobID {
			switch data.Status {
			case Foreground:
			case Background:
				err := syscall.Kill(pid, syscall.SIGSTOP)
				if err != nil {
					return err
				}
				err = syscall.Kill(pid, syscall.SIGCONT)
				if err != nil {
					return err
				}
			case Stopped:
				err := syscall.Kill(pid, syscall.SIGCONT)
				if err != nil {
					return err
				}
			}
			pm.jobsID--
			pm.jobsCount--
			data.Status = Foreground
			pm.mu.Unlock()
			pm.Wait(pid, *data)
			return nil
		}
	}
	pm.mu.Unlock()
	return fmt.Errorf("job %d не найден", jobID)
}
