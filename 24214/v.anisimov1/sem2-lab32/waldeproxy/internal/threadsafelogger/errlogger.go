package threadsafelogger

import (
	"log"
	"os"
	"sync"
)

type ErrLogger struct {
	logger *log.Logger
	mux    *sync.Mutex
}

func InitErrLogger(prefix string) *ErrLogger {
	return &ErrLogger{logger: log.New(os.Stderr, prefix, 0), mux: &sync.Mutex{}}
}

func (el *ErrLogger) Info(format string, args ...any) {
	el.mux.Lock()
	defer el.mux.Unlock()

	if len(args) == 0 {
		el.logger.Printf(format + "\n")
	} else {
		el.logger.Printf(format+"\n", args)
	}
}
