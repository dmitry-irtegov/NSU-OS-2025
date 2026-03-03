package main

import (
	"fmt"
	"os"
	"os/signal"
	"strconv"
	"sync/atomic"
	"syscall"
)

const batchSize = 1_000_000 // Number iterations beetwen flag checking

func calcPartialPi(id, numThreads int, stopFlag *atomic.Bool, ch chan<- float64) {
	var sum float64 = 0
	iteration := 0

	for !stopFlag.Load() {
		for i := 0; i < batchSize; i++ {
			k := (iteration*batchSize+i)*numThreads + id

			term := 1.0 / float64(2*k+1)

			if k%2 != 0 {
				sum -= term
			} else {
				sum += term
			}
		}
		iteration++
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

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, os.Interrupt, syscall.SIGINT)

	go func() {
		<-sigCh
		fmt.Println("\nSIGINT received")
		stopFlag.Store(true)
	}()

	ch := make(chan float64, numOfPthreads)

	for i := 0; i < numOfPthreads; i++ {
		go calcPartialPi(i, numOfPthreads, &stopFlag, ch)
	}

	var totalSum float64 = 0

	for i := 0; i < numOfPthreads; i++ {
		totalSum += <-ch
	}
	close(ch)

	pi := totalSum * 4.0

	fmt.Printf("Pi = %.15f\n", pi)
}
