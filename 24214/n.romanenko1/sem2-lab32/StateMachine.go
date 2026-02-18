package main

import (
	"errors"
	"fmt"
	"syscall"
	"time"

	"golang.org/x/sys/unix"
)

type StateMachine struct {
	clientFd int
	siteFd   int

	buffer   []byte
	request  []byte
	response []byte

	url  string
	host string
	ip   []byte
	port int

	cache *Cache
}

func (sm *StateMachine) createPortToSite() error {
	siteFd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return err
	}

	ip, port := resolveHostPort(sm.host, 80)
	siteAddress := &unix.SockaddrInet4{Port: port, Addr: ip}

	err = unix.Connect(siteFd, siteAddress)

	if err == nil {
		sm.siteFd = siteFd
	} else {
		return err
	}

	return nil
}

func (sm *StateMachine) handleClientRead() error {
	buf := make([]byte, 4096)
	n, err := syscall.Read(sm.clientFd, buf)

	if err == nil {
		if n == 0 {
			return errors.New("Client disconnected")
		}

		sm.request = buf
		url, host, ok := parseRequest(sm.request)

		if !ok {
			return errors.New("invalid HTTP request")
		}

		sm.url = url
		sm.host = host

		return nil

	} else {
		return err
	}
}

func (sm *StateMachine) tryCache() bool {
	if entry, exist := sm.cache.Get(sm.url); exist && time.Since(entry.Timestamp) < 1*time.Minute {
		fmt.Printf("Get response from cache with url: %s\n", sm.url)
		sm.response = entry.Data
		return true
	}

	return false
}

func (sm *StateMachine) connectToSite() error {
	err := sm.createPortToSite()
	if err != nil {
		return err
	}

	return nil
}

func (sm *StateMachine) writeRequestToSite() error {
	n, err := unix.Write(sm.siteFd, sm.request)
	if err != nil {
		return err
	}

	if n <= 0 {
		return errors.New("Site didn't receive request")
	}

	return nil
}

func (sm *StateMachine) readResponseFromSite() error {
	buf := make([]byte, 4096)
	n, err := unix.Read(sm.siteFd, buf)

	if err != nil {
		return err
	}

	if n <= 0 {
		return errors.New("Empty response from site")
	} else {
		sm.response = make([]byte, n)
		copy(sm.response, buf)
		return nil
	}
}

func (sm *StateMachine) writeToClient() error {
	n, err := unix.Write(sm.clientFd, sm.response)
	if err != nil {
		return err
	}

	if n <= 0 {
		return errors.New("Client didn't receive data")
	} else {
		if _, exist := sm.cache.Get(sm.url); !exist {
			sm.cache.Set(sm.url, sm.response)
		}

		return nil
	}
}
