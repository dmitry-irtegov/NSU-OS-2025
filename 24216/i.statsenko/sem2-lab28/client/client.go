package client

import (
	"fmt"
	"net"
	"net/url"
	"strconv"

	"golang.org/x/sys/unix"
)

const linesPerPage = 25

var (
	prompt      = []byte("Press space to scroll down")
	clearPrompt = []byte("\r\033[K")
)

type Client struct {
	socket      int
	oldTerminal unix.Termios
	mask        unix.FdSet
	buf         []byte
	headerDone  bool
	lineCount   int
}

func NewClient(address string) (*Client, error) {
	data, err := url.Parse(address)
	if err != nil {
		return nil, err
	}
	ips, err := net.LookupIP(data.Hostname())
	if err != nil {
		return nil, err
	}
	sk, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return nil, err
	}
	port := 80
	if data.Port() != "" {
		port, err = strconv.Atoi(data.Port())
		if err != nil {
			unix.Close(sk)
			return nil, fmt.Errorf("invalid port: %v", err)
		}
	}
	addr := &unix.SockaddrInet4{Port: port}
	copy(addr.Addr[:], ips[0].To4())
	if err := unix.Connect(sk, addr); err != nil {
		unix.Close(sk)
		return nil, err
	}
	path := "/"
	if data.RequestURI() != "" {
		path = data.RequestURI()
	}
	req := fmt.Sprintf("GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, data.Hostname())
	if _, err = unix.Write(sk, []byte(req)); err != nil {
		unix.Close(sk)
		return nil, err
	}
	return &Client{socket: sk}, nil
}

func (c *Client) Run() error {
	if err := c.setRawMode(); err != nil {
		return err
	}
	defer c.restoreTerminal()

	maxFd := c.socket + 1
	socketOpen := true
	promptShown := false

	for {
		if !socketOpen && c.lineCount < linesPerPage {
			if err := c.outputLines(); err != nil {
				return err
			}
			if c.lineCount >= linesPerPage {
				continue
			}
			if len(c.buf) > 0 {
				c.buf = append(c.buf, '\r', '\n')
				_, err := unix.Write(1, c.buf)
				if err != nil {
					return err
				}
			}
			return nil
		}

		c.fdZero()
		if socketOpen {
			c.fdSet(c.socket)
		}
		if c.lineCount >= linesPerPage {
			c.fdSet(0)
			if !promptShown {
				_, err := unix.Write(1, prompt)
				if err != nil {
					return err
				}
				promptShown = true
			}
		}

		if _, err := unix.Select(maxFd, &c.mask, nil, nil, nil); err != nil {
			if err == unix.EINTR {
				continue
			}
			return err
		}

		if c.fdIsSet(0) {
			var b [1]byte
			n, err := unix.Read(0, b[:])
			if err != nil {
				return err
			}
			if n > 0 && b[0] == ' ' {
				_, err = unix.Write(1, clearPrompt)
				if err != nil {
					return err
				}
				c.lineCount = 0
				promptShown = false
				if err = c.outputLines(); err != nil {
					return err
				}
			}
		}

		if socketOpen && c.fdIsSet(c.socket) {
			var tmp [4096]byte
			n, err := unix.Read(c.socket, tmp[:])
			if n > 0 {
				c.buf = append(c.buf, tmp[:n]...)
				c.skipHeader()
				if c.lineCount < linesPerPage {
					if err := c.outputLines(); err != nil {
						return err
					}
				}
			}
			if err != nil || n == 0 {
				unix.Close(c.socket)
				socketOpen = false
			}
		}
	}
}

func (c *Client) outputLines() error {
	for c.lineCount < linesPerPage {
		idx := -1
		for i, b := range c.buf {
			if b == '\n' {
				idx = i
				break
			}
		}
		if idx < 0 {
			break
		}
		end := idx
		if end > 0 && c.buf[end-1] == '\r' {
			end--
		}
		if _, err := unix.Write(1, c.buf[:end]); err != nil {
			return err
		}
		if _, err := unix.Write(1, []byte{'\r', '\n'}); err != nil {
			return err
		}
		c.buf = c.buf[idx+1:]
		c.lineCount++
	}
	return nil
}

func (c *Client) skipHeader() {
	if c.headerDone {
		return
	}
	for i := 0; i <= len(c.buf)-4; i++ {
		if c.buf[i] == '\r' && c.buf[i+1] == '\n' && c.buf[i+2] == '\r' && c.buf[i+3] == '\n' {
			c.buf = c.buf[i+4:]
			c.headerDone = true
			return
		}
	}
}

func (c *Client) setRawMode() error {
	term, err := unix.IoctlGetTermios(0, unix.TCGETS)
	if err != nil {
		return err
	}
	c.oldTerminal = *term
	term.Lflag &^= unix.ICANON | unix.ECHO
	term.Cc[unix.VMIN] = 1
	term.Cc[unix.VTIME] = 0
	return unix.IoctlSetTermios(0, unix.TCSETS, term)
}

func (c *Client) restoreTerminal() {
	unix.IoctlSetTermios(0, unix.TCSETS, &c.oldTerminal)
}

func (c *Client) fdZero() {
	for i := range c.mask.Bits {
		c.mask.Bits[i] = 0
	}
}

func (c *Client) fdSet(fd int) {
	c.mask.Bits[fd/64] |= 1 << (uint(fd) % 64)
}

func (c *Client) fdIsSet(fd int) bool {
	return c.mask.Bits[fd/64]&(1<<(uint(fd)%64)) != 0
}
