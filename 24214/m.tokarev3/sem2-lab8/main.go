package main

import (
	"fmt"
	"os"
	"strconv"
)

const totalIterations = 1_000_000_000

func calcPartialPi(start, end int, ch chan<- float64) {
	var sum float64 = 0
	for k := start; k < end; k++ {
		term := 1.0 / float64(2*k+1)

		if k%2 != 0 {
			sum -= term
		} else {
			sum += term
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

	ch := make(chan float64, numOfPthreads)

	chunkSize := totalIterations / numOfPthreads

	for i := 0; i < numOfPthreads; i++ {
		start := i * chunkSize
		end := start + chunkSize

		if i == numOfPthreads-1 {
			end = totalIterations
		}

		go calcPartialPi(start, end, ch)
	}

	var totalSum float64 = 0

	for i := 0; i < numOfPthreads; i++ {
		totalSum += <-ch
	}
	close(ch)

	pi := totalSum * 4.0

	fmt.Printf("Pi = %.8f\n", pi)
}
