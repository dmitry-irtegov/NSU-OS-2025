package threadsafelogger

import (
	"log"
	"os"
	"sync"
)

type NormalLogger struct {
	logger *log.Logger
	mux    *sync.Mutex
}

func InitNormalLogger(prefix string) *NormalLogger {
	return &NormalLogger{logger: log.New(os.Stdout, prefix, 0), mux: &sync.Mutex{}}
}

func (nl *NormalLogger) Info(format string, args ...any) {
	nl.mux.Lock()
	defer nl.mux.Unlock()

	if len(args) == 0 {
		nl.logger.Printf(format + "\n")
	} else {
		nl.logger.Printf(format+"\n", args)
	}
}
