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

type ProcessData struct {
	Background bool
}

type Manager struct {
	processes map[int]*ProcessData // [pid]processData
	mu        sync.Mutex
	Prompt    prompt.Prompt
}

func NewProcessManager(prompt prompt.Prompt) *Manager {
	pm := &Manager{
		processes: make(map[int]*ProcessData),
		Prompt:    prompt,
	}
	go pm.handleStopSignals()
	go pm.handleInterruptSignals()
	return pm
}

// only with mu lock
func (pm *Manager) handleKillerror(err error, pid int) error {
	if err != nil {
		if !errors.Is(err, syscall.ESRCH) {
			return err
		}
		delete(pm.processes, pid)
	}
	return nil
}

// only with mu lock
func (pm *Manager) interruptFGroup(pid int) error {
	if data := pm.processes[pid]; !data.Background {

		err := syscall.Kill(pid, syscall.SIGINT)
		if pm.handleKillerror(err, pid) != nil {
			return err
		}

		fmt.Println("\nProcess", pid, "interrupted")
		delete(pm.processes, pid)
	}
	return nil
}

// only with mu lock
func (pm *Manager) toBackgroundGroup(pid int) error {
	if data := pm.processes[pid]; !data.Background {
		err := syscall.Kill(pid, syscall.SIGSTOP)
		if pm.handleKillerror(err, pid) != nil {
			return err
		}
		pm.processes[pid].Background = true
		err = syscall.Kill(pid, syscall.SIGCONT)
		if pm.handleKillerror(err, pid) != nil {
			return err
		}

		fmt.Fprint(os.Stderr, "\nProcess ", pid, " moved to background\n")
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
			fmt.Println(pid)
			err := pm.interruptFGroup(pid)
			if err != nil {
				fmt.Println("Не удалось прервать процесс", pid, "по причине:", err)
			}
		}
		pm.mu.Unlock()
	}
}

func (pm *Manager) handleStopSignals() {
	c := make(chan os.Signal, 1)

	signal.Notify(c, syscall.SIGTSTP)

	for sig := range c {
		fmt.Println("Получен сигнал: ", sig)
		pm.mu.Lock()
		for pid := range pm.processes {
			fmt.Println("переводим процесс", pid, "в фоновый режим")
			err := pm.toBackgroundGroup(pid)
			if err != nil {
				fmt.Println("Не удалось перевести процесс", pid, "в фоновый режим по причине:", err)
			}
		}
		pm.mu.Unlock()
	}
}

func (pm *Manager) KillAll() {
	pm.mu.Lock()
	for pid := range pm.processes {
		fmt.Println("kill:", pid)
		_ = syscall.Kill(pid, syscall.SIGKILL)
	}
	pm.mu.Unlock()
}

func (pm *Manager) Wait(pid int, data ProcessData) {
	pm.mu.Lock()
	pm.processes[pid] = &data
	pm.mu.Unlock()
	var ws syscall.WaitStatus
	_, err := syscall.Wait4(pid, &ws, 0, nil)
	if err != nil {
		fmt.Println("Ошибка ожидания процесса: ", pid)
	}
	pm.mu.Lock()
	delete(pm.processes, pid)
	pm.mu.Unlock()
}
