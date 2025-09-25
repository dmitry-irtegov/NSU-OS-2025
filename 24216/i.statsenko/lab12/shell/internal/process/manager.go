package process

import (
	"errors"
	"fmt"
	"os"
	"os/signal"
	"shell/internal/prompt"
	"sync"
	"syscall"
)

type ProcessManager struct {
	processes map[int]bool // [pgid]background
	mu        sync.Mutex
	Prompt    prompt.Prompt
}

func NewProcessManager(prompt prompt.Prompt) *ProcessManager {
	pm := &ProcessManager{
		processes: make(map[int]bool),
		Prompt:    prompt,
	}
	go pm.handleStopSignals()
	go pm.handleInterruptSignals()
	return pm
}

//func (pm *ProcessManager) AddGroup(pgid int, background bool) {
//	pm.mu.Lock()
//	pm.processes[pgid] = background
//	pm.mu.Unlock()
//}

// only with mu lock
func (pm *ProcessManager) handleKillerror(err error, pgid int) error {
	if err != nil {
		if !errors.Is(err, syscall.ESRCH) {
			return err
		}
		delete(pm.processes, pgid)
	}
	return nil
}

// only with mu lock
func (pm *ProcessManager) interruptFGroup(pgid int) error {
	if background, _ := pm.processes[pgid]; !background {
		err := syscall.Kill(pgid, syscall.SIGINT)
		if pm.handleKillerror(err, pgid) != nil {
			return err
		}
		fmt.Fprint(os.Stderr, "\nProcess", pgid, "interrupted")
		delete(pm.processes, pgid)
	}
	return nil
}

// only with mu lock
func (pm *ProcessManager) toBackgroundGroup(pgid int) error {
	if background, _ := pm.processes[pgid]; !background {
		err := syscall.Kill(pgid, syscall.SIGSTOP)
		if pm.handleKillerror(err, pgid) != nil {
			return err
		}
		pm.processes[pgid] = true
		err = syscall.Kill(pgid, syscall.SIGCONT)
		if pm.handleKillerror(err, pgid) != nil {
			return err
		}
		fmt.Fprint(os.Stderr, "\nProcess ", pgid, " moved to background\n")
	}
	return nil
}

func (pm *ProcessManager) handleInterruptSignals() {
	c := make(chan os.Signal, 1)
	signal.Notify(c, syscall.SIGINT, syscall.SIGQUIT)
	for sig := range c {
		fmt.Println("Получен сигнал:", sig)
		if sig == syscall.SIGQUIT {
			continue
		}
		pm.mu.Lock()
		for pgid := range pm.processes {
			fmt.Println(pgid)
			err := pm.interruptFGroup(pgid)
			if err != nil {
				fmt.Println("Не удалось прервать процесс", pgid, "по причине:", err)
			}
		}
		pm.mu.Unlock()
	}
}

func (pm *ProcessManager) handleStopSignals() {
	c := make(chan os.Signal, 1)

	signal.Notify(c, syscall.SIGTSTP)

	for range c {
		fmt.Println("Получен сигнал: SIGTSTP")
		pm.mu.Lock()
		for pgid := range pm.processes {
			err := pm.toBackgroundGroup(pgid)
			if err != nil {
				fmt.Println("Не удалось перевести процесс", pgid, "в фоновый режим по причине:", err)
			}
		}
		pm.mu.Unlock()
	}
}

func (pm *ProcessManager) KillAll() {
	pm.mu.Lock()
	for pgid := range pm.processes {
		fmt.Println("kill:", pgid)
		_ = syscall.Kill(pgid, syscall.SIGKILL)
	}
	pm.mu.Unlock()
}

func (pm *ProcessManager) Wait(pgid int, background bool) {
	pm.mu.Lock()
	pm.processes[pgid] = background
	pm.mu.Unlock()
	var ws syscall.WaitStatus
	_, err := syscall.Wait4(pgid, &ws, 0, nil)
	if err != nil {
		fmt.Println("Ошибка ожидания процесса: ", pgid)
	}
	pm.mu.Lock()
	delete(pm.processes, pgid)
	pm.mu.Unlock()
}
