package client

import (
	"bytes"
	"fmt"
	"waldeproxy/internal/cache"

	"golang.org/x/sys/unix"
)

func Handle(ccontext *ClientContext) {
	defer func() {
		unix.Close(ccontext.socketFd)
		ccontext.normalLogger.Info("CLIENT %d: closing connection", ccontext.clientid)
	}()
	request, err := readRequest(ccontext.socketFd)

	if err != nil {
		return
	}

	nomalizedRequest := prepareRequest(request)

	requestParams, err := parseRequest(string(nomalizedRequest))
	if err != nil {
		return
	}

	key := &cache.CacheKey{
		Host:     requestParams.host,
		Resource: requestParams.resource,
	}

	for {
		if response, err := ccontext.cache.GetData(key); err != nil {
			if err == cache.EmptyDoesNotExistError {
				break
			}

			if err == cache.IsNotRelevantNowError {
				continue
			}
		} else {
			ccontext.normalLogger.Info("CLIENT %d: get response from the cache", ccontext.clientid)
			if err := sendHTTPmessage(ccontext.socketFd, response); err != nil {
				ccontext.errLogger.Info("CLIENT %d: cannot send reponse on my socket: %v", ccontext.clientid, err)
			}

			return
		}
	}

	if err := ccontext.cache.RegisterEntry(key); err != nil {
		ccontext.errLogger.Info("CLIENT %d: cannot register entry in the cache: %v", ccontext.clientid, err)
	}

	response, err := handleUpstream(nomalizedRequest)
	if err != nil {
		ccontext.errLogger.Info("CLIENT %d: cannot get response from upstream: %v", ccontext.clientid, err)
		return
	}

	ccontext.normalLogger.Info("CLIENT %d: sending response to myself...", ccontext.clientid)
	err = sendHTTPmessage(ccontext.socketFd, response)
	if err != nil {
		ccontext.errLogger.Info("CLIENT %d: cannot send reponse on my socket: %v", ccontext.clientid, err)
	}

	ccontext.normalLogger.Info("CLIENT %d: saving response in the cache...", ccontext.clientid)
	if responseValidCheck(response) {
		if err := ccontext.cache.AddResponseToEntry(
			key,
			response,
			extractMIME(response),
		); err != nil {
			ccontext.errLogger.Info("CLIENT %d: cannot save the reponse in the cache: %v", ccontext.clientid, err)
		}
	} else {
		ccontext.normalLogger.Info("CLIENT %d: not 200 code - do not save to the cache", ccontext.clientid)
		ccontext.cache.DeleteEntry(key)
	}
}

func readRequest(socketfd int) ([]byte, error) {
	buffer := make([]byte, 1024)
	payloadSize := 0

	for headerEndIdx := bytes.Index(buffer[:payloadSize], []byte("\r\n\r\n")); headerEndIdx == -1; headerEndIdx = bytes.Index(buffer[:payloadSize], []byte("\r\n\r\n")) {
		if payloadSize == len(buffer) {
			newBuf := make([]byte, len(buffer)*2)
			copy(newBuf, buffer)
			buffer = newBuf
		}

		n, err := unix.Read(socketfd, buffer[payloadSize:])
		if err != nil {
			return nil, fmt.Errorf("failed to read client's request: %v", err)
		}

		if n == 0 {
			return nil, fmt.Errorf("connection closed")
		}

		payloadSize += n
	}

	copied := make([]byte, payloadSize)
	copy(copied, buffer[:payloadSize])
	return copied, nil
}

func handleUpstream(request []byte) ([]byte, error) {
	upstreamfd, err := connectUpstream(string(request))
	if err != nil {
		return nil, fmt.Errorf("failed to connect to upstream: %v", err)
	}
	defer unix.Close(upstreamfd)

	if err := sendHTTPmessage(upstreamfd, request); err != nil {
		return nil, fmt.Errorf("failed to send request to upstream: %v", err)
	}

	response, err := readResponse(upstreamfd)
	if err != nil {
		return nil, fmt.Errorf("failed to read upstream's response: %v", err)
	}

	return response, nil
}

func readResponse(upstreamfd int) ([]byte, error) {
	buffer := make([]byte, 1024)
	payloadSize, headerEndIdx := 0, 0

	for headerEndIdx = bytes.Index(buffer[:payloadSize], []byte("\r\n\r\n")); headerEndIdx == -1; headerEndIdx = bytes.Index(buffer[:payloadSize], []byte("\r\n\r\n")) {
		if payloadSize == len(buffer) {
			newBuf := make([]byte, len(buffer)*2)
			copy(newBuf, buffer)
			buffer = newBuf
		}

		n, err := unix.Read(upstreamfd, buffer[payloadSize:])
		if err != nil {
			return nil, fmt.Errorf("failed to read client's request: %v", err)
		}

		payloadSize += n
	}

	bodySize := getExpectedBodySize(buffer[:headerEndIdx])

	expectedTotalSize := headerEndIdx + 4 + bodySize

	for payloadSize < expectedTotalSize {
		if payloadSize == len(buffer) {
			newBuf := make([]byte, len(buffer)*2)
			copy(newBuf, buffer)
			buffer = newBuf
		}

		n, err := unix.Read(upstreamfd, buffer[payloadSize:])
		if err != nil {
			return nil, fmt.Errorf("failed to read client's request: %v", err)
		}

		payloadSize += n
	}

	copied := make([]byte, payloadSize)
	copy(copied, buffer[:payloadSize])
	return copied, nil
}

func sendHTTPmessage(dstSocket int, message []byte) error {
	wrote := 0

	for n, err := unix.Write(dstSocket, message[wrote:]); wrote != len(message); n, err = unix.Write(dstSocket, message[wrote:]) {
		if err != nil {
			return fmt.Errorf("cannot write to the socket")
		} else {
			wrote += n
		}
	}

	return nil
}
