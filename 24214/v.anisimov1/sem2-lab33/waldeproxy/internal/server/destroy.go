package server

import "golang.org/x/sys/unix"

func (s *Server) Destroy() {
	defer func() {
		if r := recover(); r != nil {
			s.errLogger.Printf("panic occured!\n")
		}
	}()

	unix.Close(s.socket)
	if s.socketChannel != nil {
		close(s.socketChannel)
	}
}
