package server

import (
	"errors"
	"log"

	"golang.org/x/sys/unix"
)

var (
	port         = 8080
	serverIPAddr = [4]byte{0, 0, 0, 0}
)

type session interface {
	getType() sessionType
}

type server struct {
	activeClients int
	socket        int
	limit         int
	pollfdset     []unix.PollFd
	sessions      map[int]session
	respCache     *cache
	errLogger     *log.Logger
	normalLogger  *log.Logger
}

type clientSession struct {
	socketfd   int
	state      sessionState
	buffer     []byte
	bufferSize int
	wroteBytes int
}

func (*clientSession) getType() sessionType {
	return typeClient
}

type upstreamSession struct {
	socketfd   int
	state      sessionState
	buffer     []byte
	bufferSize int
	wroteBytes int
	key        *cacheKey
	entry      *cacheEntry
}

func (*upstreamSession) getType() sessionType {
	return typeUpstream
}

type sessionState int

const (
	readClientRequest sessionState = iota
	sendResponseToClient

	sendRequestToUpstream
	readUpstremResponse
)

type sessionType int

const (
	typeClient sessionType = iota
	typeUpstream
)

var errSessionComplete error = errors.New("session was completed successfully")
