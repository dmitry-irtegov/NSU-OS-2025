package worker

import (
	"bytes"
	"fmt"
	"waldeproxy/internal/cache"
	"waldeproxy/internal/client"
	"waldeproxy/internal/request"
	"waldeproxy/internal/response"

	"golang.org/x/sys/unix"
)

func (w *Worker) handleClient(cc *client.ClientContext) error {
	if state := cc.State; state == client.Read {
		if err := w.read(cc); err != nil {
			return fmt.Errorf("read client's request: %v", err)
		}
	} else if state == client.SendRequestToUpstream {
		if err := w.sendRequestToUpstream(cc); err != nil {
			return fmt.Errorf("send response to client from cache: %v", err)
		}
	} else if state == client.SendResponseFromCache {
		if err := w.sendFromCache(cc); err != nil {
			return fmt.Errorf("send response to client from cache: %v", err)
		}
	} else {
		if err := w.sendFromUpstream(cc); err != nil {
			return fmt.Errorf("send response to client from upstream: %v", err)
		}
	}

	return nil
}

func (w *Worker) read(cc *client.ClientContext) error {
	revents := w.pollArea[cc.ClientPollIdx].Revents

	if revents&unix.POLLIN != 0 {
		if cc.PayloadSize == len(cc.Buffer) {
			newBuf := make([]byte, len(cc.Buffer)*2)
			copy(newBuf, cc.Buffer)
			cc.Buffer = newBuf
		}

		n, err := unix.Read(
			cc.ClientFd,
			cc.Buffer[cc.PayloadSize:],
		)

		if err == unix.EAGAIN {
			return nil
		}

		if err != nil {
			return fmt.Errorf("cannot get data from a socket: %w", err)
		}

		if n == 0 {
			return fmt.Errorf("client closed connection")
		}

		cc.PayloadSize += n
		cc.Buffer = cc.Buffer[:cc.PayloadSize]

		if headerEndIdx := bytes.Index(cc.Buffer, []byte("\r\n\r\n")); headerEndIdx != -1 {
			normalizedRequest := prepareRequest(cc.Buffer)

			requestMeta, err := request.ParseRequest(normalizedRequest)
			if err != nil {
				return fmt.Errorf("error parse request: %v", err)
			}

			cc.Key = cache.NewCacheKey(
				requestMeta.GetHost(),
				requestMeta.GetResource(),
			)

			if !w.cache.IsEntryExists(cc.Key) {
				if err := w.openUpstreamConnection(cc, requestMeta, normalizedRequest); err != nil {
					return fmt.Errorf("cannot open upstream connection: %w", err)
				} else {
					cc.State = client.SendRequestToUpstream
					w.normalLogger.Println("getting from upstream connection in streaming mode")
				}
			} else {
				cc.State = client.SendResponseFromCache
				w.pollArea[cc.ClientPollIdx].Events = 0
				cc.Waiting = true
				w.normalLogger.Println("getting from cache in streaming mode")
			}
		}
	}
	return nil
}

func (w *Worker) sendRequestToUpstream(cc *client.ClientContext) error {
	revents := w.pollArea[cc.UpstreamPollIdx].Revents

	if revents&unix.POLLOUT != 0 {
		n, err := unix.Write(
			cc.UpstreamFd,
			cc.Buffer[cc.WroteBytes:cc.PayloadSize],
		)

		if err == unix.EAGAIN {
			return nil
		}

		if err != nil {
			return fmt.Errorf("cannot write to client's socket: %w", err)
		}

		cc.WroteBytes += n

		if cc.WroteBytes == cc.PayloadSize {
			cc.State = client.SendResponseFromUpstream
			cc.WroteBytes = 0
			cc.PayloadSize = 0
			cc.Buffer = make([]byte, 1024)
			w.pollArea[cc.UpstreamPollIdx].Events = unix.POLLIN
			w.pollArea[cc.ClientPollIdx].Events = 0
			w.normalLogger.Println("request was sent to upstream successfully")
		}
	}

	return nil
}

func (w *Worker) sendFromCache(cc *client.ClientContext) error {
	revents := w.pollArea[cc.ClientPollIdx].Revents

	if revents&unix.POLLOUT != 0 {
		chunk, err := w.cache.GetData(cc.Key, cc.WroteBytes)
		if err == cache.NotEnoughData {
			cc.Waiting = true
			w.pollArea[cc.ClientPollIdx].Events = 0
			return nil
		}
		if err != nil {
			return fmt.Errorf("cache access error: %v", err)
		}

		if len(chunk) == 0 {
			cc.Waiting = true
			w.pollArea[cc.ClientPollIdx].Events = 0
			return nil
		}

		n, err := unix.Write(cc.ClientFd, chunk)
		if err != nil {
			if err == unix.EAGAIN {
				return nil
			}
			return fmt.Errorf("cannot write to chunk: %w", err)
		}
		cc.WroteBytes += n

		isFull, _ := w.cache.GetFull(cc.Key)
		hasNew, _ := w.cache.HasNewData(cc.Key, cc.WroteBytes)

		if isFull && !hasNew {
			w.normalLogger.Println("all data from cache sent successfully")
			w.closeClientSession(cc)
		} else if n <= len(chunk) {
			cc.Waiting = true
			w.pollArea[cc.ClientPollIdx].Events = 0
		}
	}

	return nil
}

func (w *Worker) sendFromUpstream(cc *client.ClientContext) error {
	var upstreamRevents int16
	var clientRevents int16

	if cc.UpstreamPollIdx != -1 {
		upstreamRevents = w.pollArea[cc.UpstreamPollIdx].Revents
	}

	if cc.ClientPollIdx != -1 {
		clientRevents = w.pollArea[cc.ClientPollIdx].Revents
	}

	if upstreamRevents&unix.POLLIN != 0 {
		if cc.PayloadSize == len(cc.Buffer) {
			newBuf := make([]byte, len(cc.Buffer)*2)
			copy(newBuf, cc.Buffer)
			cc.Buffer = newBuf
		}

		n, err := unix.Read(
			cc.UpstreamFd,
			cc.Buffer[cc.PayloadSize:],
		)

		if err == unix.EAGAIN {
			return nil
		}

		if err != nil {
			if cc.WritingToCache {
				w.cache.DeleteEntry(cc.Key)
			}

			unix.Close(cc.UpstreamFd)
			cc.UpstreamFd = -1
			w.pollArea[cc.UpstreamPollIdx].Events = 0
			w.pollArea[cc.UpstreamPollIdx].Revents = 0
			w.pollArea[cc.UpstreamPollIdx].Fd = -1

			return fmt.Errorf("cannot get chunk from upstream: %w", err)
		}

		if n == 0 {
			if cc.WritingToCache {
				w.cache.AcceptEntry(cc.Key)
			}

			unix.Close(cc.UpstreamFd)
			cc.UpstreamFd = -1
			w.pollArea[cc.UpstreamPollIdx].Events = 0
			w.pollArea[cc.UpstreamPollIdx].Revents = 0
			w.pollArea[cc.UpstreamPollIdx].Fd = -1

			w.normalLogger.Printf("upstream-host %s closed connection", cc.Key.GetHost())
		}

		if cc.WritingToCache {
			w.cache.PutDataIntoCache(cc.Key, cc.Buffer[cc.PayloadSize:cc.PayloadSize+n])
		}

		cc.PayloadSize += n

		if n > 0 {
			w.pollArea[cc.ClientPollIdx].Events = unix.POLLOUT
		}

		headerEndIdx := bytes.Index(cc.Buffer[:cc.PayloadSize], []byte("\r\n\r\n"))

		if headerEndIdx == -1 {
			return nil
		} else {
			if !cc.HeaderProcessed {
				is200 := response.ResponseValidCheck(cc.Buffer[:headerEndIdx])
				mime := response.ExtractMIME(cc.Buffer[:headerEndIdx])

				if is200 {
					if err := w.cache.RegisterEntry(cc.Key); err == nil {
						w.cache.SetMime(cc.Key, mime)
						w.cache.PutDataIntoCache(cc.Key, cc.Buffer[:cc.PayloadSize])
						cc.WritingToCache = true
					}
				}

				cc.HeaderProcessed = true
			}
		}

		bodySize := response.GetExpectedBodySize(cc.Buffer[:headerEndIdx])

		expectedTotalSize := headerEndIdx + 4 + bodySize

		if bodySize != -1 && cc.PayloadSize == expectedTotalSize {
			if cc.WritingToCache {
				w.cache.AcceptEntry(cc.Key)
			}

			unix.Close(cc.UpstreamFd)
			delete(w.upstreamMap, cc.UpstreamFd)

			w.pollArea[cc.UpstreamPollIdx].Events = 0
			w.pollArea[cc.UpstreamPollIdx].Revents = 0
			w.pollArea[cc.UpstreamPollIdx].Fd = -1

			cc.UpstreamFd = -1
			cc.UpstreamPollIdx = -1
		}
	}

	if clientRevents&unix.POLLOUT != 0 {
		if cc.WroteBytes >= cc.PayloadSize {
			if cc.UpstreamFd == -1 {
				w.normalLogger.Printf("closing connection: full response sent\n")
				w.closeClientSession(cc)
			}
		} else {
			n, err := unix.Write(cc.ClientFd, cc.Buffer[cc.WroteBytes:cc.PayloadSize])

			if err != nil {
				if err == unix.EAGAIN {
					return nil
				}
				return fmt.Errorf("cannot write to chunk: %w", err)
			}

			cc.WroteBytes += n
		}
	}

	return nil
}

func (w *Worker) openUpstreamConnection(
	cc *client.ClientContext,
	requestMeta *request.RequestMeta,
	request []byte,
) error {
	if upstrSock, err := w.connectUpstream(requestMeta); err != nil {
		return fmt.Errorf("cannot connect to upstream: %v", err)
	} else {
		cc.UpstreamFd = upstrSock
		cc.UpstreamPollIdx = len(w.pollArea)

		w.pollArea = append(w.pollArea, unix.PollFd{
			Fd:      int32(upstrSock),
			Events:  unix.POLLOUT,
			Revents: 0,
		})

		w.upstreamMap[upstrSock] = cc
	}

	return nil
}
