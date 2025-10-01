package execute

import (
	"errors"
	"os"
)

type CmdRequest struct {
	Cmds  []Command
	Ncmds int
}

type Command struct {
	Cmdargs                  []string
	Infile, Outfile, Appfile string
	Bkgrnd                   bool
	Pipe                     bool
	InPipe, OutPipe          *os.File
}

var ErrExit = errors.New("exit")
