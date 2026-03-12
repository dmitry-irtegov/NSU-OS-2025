package client

import (
	"fmt"
	"laba/parser"
	"net"
	"net/url"
	"strconv"
	"syscall"
	"unsafe"
)

var (
	_continue      = []byte("Press space to scroll down")
	_clearContinue = []byte("\r\033[K")
)

const _requestString = "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n"

type Client struct {
	socket      int
	oldTerminal syscall.Termios
	mask        syscall.FdSet
	pars        *parser.Parser
}

func NewClient(pars *parser.Parser, address string) (*Client, error) {
	data, err := url.Parse(address)
	if err != nil {
		return nil, err
	}
	ip, err := net.LookupIP(data.Hostname())
	if err != nil {
		return nil, err
	}
	sk, err := syscall.Socket(syscall.AF_INET, syscall.SOCK_STREAM, 0)
	if err != nil {
		return nil, err
	}
	var port = 80
	if data.Port() != "" {
		port, err = strconv.Atoi(data.Port())
		if err != nil {
			return nil, fmt.Errorf("invalid port number: %w", err)
		}
	}
	connInfo := syscall.SockaddrInet4{Port: port}
	copy(connInfo.Addr[:], ip[0].To4())
	if err := syscall.Connect(sk, &connInfo); err != nil {
		return nil, err
	}
	ur := "/"
	if data.RequestURI() != "" {
		ur = data.RequestURI()
	}
	req := fmt.Sprintf(_requestString, ur, data.Hostname())
	_, err = syscall.Write(sk, []byte(req))
	if err != nil {
		return nil, err
	}
	return &Client{socket: sk, pars: pars}, nil
}

func getTerminalWidth() int {
	type winsize struct {
		Row    uint16
		Col    uint16
		Xpixel uint16
		Ypixel uint16
	}
	ws := winsize{}
	_, _, errno := syscall.Syscall(syscall.SYS_IOCTL, uintptr(syscall.Stdout),
		uintptr(syscall.TIOCGWINSZ), uintptr(unsafe.Pointer(&ws)))
	if errno != 0 || ws.Col == 0 {
		return 80
	}
	return int(ws.Col)
}

func (c *Client) Run() error {
	if err := c.terminalConfig(); err != nil {
		return err
	}
	defer c.resetTerminalConfig()

	c.pars.SetWidth(getTerminalWidth())
	maxFd := c.socket + 1
	socketOpen := true
	promptShown := false

	for {
		if !socketOpen && !c.pars.IsMaxCountGiven() {
			if c.pars.Len() > 0 {
				c.pars.Flush()
				strs, _ := c.pars.Parse(nil)
				if err := c.writeLines(strs); err != nil {
					return err
				}
				continue
			}
			return nil
		}

		c.fdZero()
		if socketOpen {
			c.fdSet(c.socket)
		}
		if c.pars.IsMaxCountGiven() {
			c.fdSet(syscall.Stdin)
			if !promptShown {
				if _, err := syscall.Write(syscall.Stdout, _continue); err != nil {
					return err
				}
				promptShown = true
			}
		} else {
			promptShown = false
		}

		if _, err := syscall.Select(maxFd, &c.mask, nil, nil, nil); err != nil {
			return err
		}

		if c.fdIsSet(syscall.Stdin) {
			if err := c.readTerminal(); err != nil {
				return err
			}
			strs, _ := c.pars.Parse(nil)
			if err := c.writeLines(strs); err != nil {
				return err
			}
		}

		if socketOpen && c.fdIsSet(c.socket) {
			buf := make([]byte, 4096)
			n, err := syscall.Read(c.socket, buf)
			if n > 0 {
				strs, _ := c.pars.Parse(buf[:n])
				if err := c.writeLines(strs); err != nil {
					return err
				}
			}
			if err != nil || n == 0 {
				syscall.Close(c.socket)
				socketOpen = false
			}
		}
	}
}

func (c *Client) terminalConfig() error {
	term := syscall.Termios{}
	_, _, errno := syscall.Syscall(syscall.SYS_IOCTL, uintptr(syscall.Stdin),
		uintptr(syscall.TCGETS), uintptr(unsafe.Pointer(&term)))
	c.oldTerminal = term
	if errno != 0 {
		return errno
	}
	term.Lflag &^= syscall.ICANON | syscall.ECHO
	_, _, errno = syscall.Syscall(syscall.SYS_IOCTL, uintptr(syscall.Stdin),
		uintptr(syscall.TCSETS), uintptr(unsafe.Pointer(&term)))
	if errno != 0 {
		return errno
	}
	return nil
}

func (c *Client) resetTerminalConfig() error {
	_, _, errno := syscall.Syscall(syscall.SYS_IOCTL, uintptr(syscall.Stdin),
		uintptr(syscall.TCSETS), uintptr(unsafe.Pointer(&c.oldTerminal)))
	if errno != 0 {
		return errno
	}
	return nil
}

func (c *Client) readTerminal() error {
	buf := make([]byte, 1)
	n, err := syscall.Read(syscall.Stdin, buf)
	if err != nil {
		return err
	}
	if n > 0 {
		if buf[0] == ' ' {
			_, err = syscall.Write(syscall.Stdout, _clearContinue)
			if err != nil {
				return err
			}
			c.pars.ResetCountGiven()
		}
	}
	return nil
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

func (c *Client) writeLines(lines []string) error {
	for _, line := range lines {
		if _, err := syscall.Write(syscall.Stdout, []byte(line+"\r\n")); err != nil {
			return err
		}
	}
	return nil
}
