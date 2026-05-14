package worker

import (
	"waldeproxy/internal/client"

	"golang.org/x/sys/unix"
)

func (w *Worker) step() {
	if len(w.pollArea) == 0 {
		newClient := <-w.clientSourse
		w.registerNewClient(newClient)
		return
	}

	select {
	case newClient := <-w.clientSourse:
		w.registerNewClient(newClient)
	default:
	}

	for pollfdIdx, pollfd := range w.pollArea {
		if pollfd.Fd != -1 && pollfd.Events == 0 {
			if cc, ok := w.clientMap[int(pollfd.Fd)]; ok && cc.Waiting {
				if res, err := w.cache.HasNewData(cc.Key, cc.PayloadSize); err != nil {
					w.closeClientSession(cc)
				} else {
					if res {
						w.pollArea[pollfdIdx].Events = unix.POLLOUT
					}
				}
			}
		}
	}

	_, err := unix.Poll(w.pollArea, 500)

	if err == unix.EINTR {
		return
	}

	if err != nil {
		w.errLogger.Printf("[poll] system-call cannot handle events: %v", err)
		return
	}

	for pollfdIdx := 0; pollfdIdx < len(w.pollArea); pollfdIdx++ {
		pollfd := w.pollArea[pollfdIdx]

		if pollfd.Revents == 0 {
			continue
		}

		cc, ok := w.clientMap[int(pollfd.Fd)]
		if !ok {
			cc, ok = w.upstreamMap[int(pollfd.Fd)]
		}

		if cc == nil {
			w.pollArea[pollfdIdx].Revents = 0
			continue
		}

		if pollfd.Revents&(unix.POLLHUP|unix.POLLERR|unix.POLLNVAL) != 0 {
			w.closeClientSession(cc)
			continue
		}

		if err := w.handleClient(cc); err != nil {
			w.closeClientSession(cc)
			w.errLogger.Printf("handle client: %v\n", err)
		}

		w.pollArea[pollfdIdx].Revents = 0
	}

	w.clearPollArea()
}

func (w *Worker) registerNewClient(clientSocketFd int) {
	newClientSession := &client.ClientContext{
		ClientFd:        clientSocketFd,
		ClientPollIdx:   len(w.pollArea),
		UpstreamFd:      -1,
		UpstreamPollIdx: -1,
		State:           client.Read,
		Buffer:          make([]byte, 1024),
		PayloadSize:     0,
		WroteBytes:      0,
		Waiting:         false,
		HeaderProcessed: false,
		WritingToCache:  false,
	}

	w.pollArea = append(w.pollArea, unix.PollFd{
		Fd:      int32(clientSocketFd),
		Events:  unix.POLLIN,
		Revents: 0,
	})

	w.clientMap[clientSocketFd] = newClientSession
}
