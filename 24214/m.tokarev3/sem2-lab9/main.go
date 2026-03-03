package main

import (
	"fmt"
	"os"
	"os/signal"
	"strconv"
	"sync/atomic"
	"syscall"
)

const batchSize int64 = 1_000_000

func calcPartialPi(stopFlag *atomic.Bool, globalBlock *atomic.Int64, ch chan<- float64) {
	var sum float64 = 0

	for {
		if stopFlag.Load() {
			break
		}

		blockID := globalBlock.Add(1) - 1
		start := blockID * batchSize
		end := start + batchSize

		for k := start; k < end; k += 2 {
			sum += 1.0/float64(2*k+1) - 1.0/float64(2*k+3)
		}
	}

	ch <- sum
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: go run main.go {num of goroutines}")
		return
	}

	numOfPthreads, err := strconv.Atoi(os.Args[1])
	if err != nil || numOfPthreads <= 0 {
		fmt.Println("Num of goroutines error: incorrect value")
		return
	}

	var stopFlag atomic.Bool
	var globalBlock atomic.Int64

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, os.Interrupt, syscall.SIGINT)

	go func() {
		<-sigCh
		fmt.Println("\nSIGINT received")
		stopFlag.Store(true)
	}()

	ch := make(chan float64, numOfPthreads)

	for i := 0; i < numOfPthreads; i++ {
		go calcPartialPi(&stopFlag, &globalBlock, ch)
	}

	var totalSum float64 = 0

	for i := 0; i < numOfPthreads; i++ {
		totalSum += <-ch
	}
	close(ch)

	pi := totalSum * 4.0

	fmt.Printf("Pi =      %.8f\n", pi)
	fmt.Printf("Real Pi = 3.14159265\n")
}
