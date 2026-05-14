package worker

import "golang.org/x/sys/unix"

func (w *Worker) clearPollArea() {
	active := make([]unix.PollFd, 0)
	for _, pfd := range w.pollArea {
		if pfd.Fd != -1 {
			if cc, ok := w.clientMap[int(pfd.Fd)]; ok {
				cc.ClientPollIdx = len(active)
			} else if cc, ok = w.upstreamMap[int(pfd.Fd)]; ok {
				cc.UpstreamPollIdx = len(active)
			}

			active = append(active, pfd)
		}
	}

	w.pollArea = active
}
