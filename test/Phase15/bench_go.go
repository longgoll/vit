package main

import (
	"fmt"
	"time"
)

func fib(n uint64) uint64 {
	if n <= 1 {
		return n
	}
	return fib(n-1) + fib(n-2)
}

func matrixMult(n int) {
	a := make([][]float64, n)
	b := make([][]float64, n)
	c := make([][]float64, n)
	for i := 0; i < n; i++ {
		a[i] = make([]float64, n)
		b[i] = make([]float64, n)
		c[i] = make([]float64, n)
		for j := 0; j < n; j++ {
			a[i][j] = 1.0
			b[i][j] = 2.0
		}
	}

	for i := 0; i < n; i++ {
		for k := 0; k < n; k++ {
			for j := 0; j < n; j++ {
				c[i][j] += a[i][k] * b[k][j]
			}
		}
	}
}

func main() {
	t1 := time.Now()
	f := fib(42)
	fibMs := float64(time.Since(t1).Microseconds()) / 1000.0

	t2 := time.Now()
	matrixMult(500)
	matMs := float64(time.Since(t2).Microseconds()) / 1000.0

	fmt.Printf("Fibonacci(42): %d in %.2f ms\n", f, fibMs)
	fmt.Printf("Matrix 500x500: in %.2f ms\n", matMs)
}
