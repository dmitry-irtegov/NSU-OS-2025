package worker

import (
	"waldeproxy/internal/client"

	"golang.org/x/sys/unix"
)

func (w *Worker) closeClientSession(cc *client.ClientContext) {
	if cc.ClientFd != -1 {
		unix.Close(cc.ClientFd)
		w.pollArea[cc.ClientPollIdx].Fd = -1
		w.pollArea[cc.ClientPollIdx].Events = 0
		w.pollArea[cc.ClientPollIdx].Revents = 0
	}

	if cc.UpstreamFd != -1 {
		unix.Close(cc.UpstreamFd)
		w.pollArea[cc.UpstreamPollIdx].Fd = -1
		w.pollArea[cc.UpstreamPollIdx].Events = 0
		w.pollArea[cc.UpstreamPollIdx].Revents = 0
	}

	delete(w.upstreamMap, cc.UpstreamFd)

	delete(w.clientMap, cc.ClientFd)
}
