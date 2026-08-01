#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static unsigned long long fib(unsigned long long n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

static void matrix_mult(int n) {
    double** a = (double**)malloc(n * sizeof(double*));
    double** b = (double**)malloc(n * sizeof(double*));
    double** c = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        a[i] = (double*)malloc(n * sizeof(double));
        b[i] = (double*)malloc(n * sizeof(double));
        c[i] = (double*)calloc(n, sizeof(double));
        for (int j = 0; j < n; j++) {
            a[i][j] = 1.0;
            b[i][j] = 2.0;
        }
    }

    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            for (int j = 0; j < n; j++) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }

    for (int i = 0; i < n; i++) {
        free(a[i]);
        free(b[i]);
        free(c[i]);
    }
    free(a);
    free(b);
    free(c);
}

int main() {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    unsigned long long f = fib(42);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double fib_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    matrix_mult(500);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double mat_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("Fibonacci(42): %llu in %.2f ms\n", f, fib_ms);
    printf("Matrix 500x500: in %.2f ms\n", mat_ms);
    return 0;
}
