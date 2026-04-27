package controller

import (
	"waldeproxy/internal/client"
	"waldeproxy/internal/server"
	"waldeproxy/internal/threadsafelogger"
)

func Run() {
	errLogger := threadsafelogger.InitErrLogger("<Application error> ")
	normalLogger := threadsafelogger.InitNormalLogger("<Application info> ")

	server, err := server.InitServer(1000, errLogger, normalLogger)
	if err != nil {
		errLogger.Info("CONTROLLER: cannot create server %v", err)
		return
	}

	normalLogger.Info("server turned on successfully")

	for counter := 0; ; counter++ {
		ccontext, err := server.AcceptClient(counter)

		if err != nil {
			errLogger.Info("CONTROLLER: server couldn't accept the client: %v", err)
			continue
		}

		normalLogger.Info("new client with id %d accepted", counter)
		go client.Handle(ccontext)
	}
}
