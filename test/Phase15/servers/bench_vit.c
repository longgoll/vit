#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static unsigned long long fib(unsigned long long n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
static void* alloc_aligned64(size_t size) {
    return _aligned_malloc(size, 64);
}
static void free_aligned64(void* ptr) {
    _aligned_free(ptr);
}
#else
static void* alloc_aligned64(size_t size) {
    void* ptr = NULL;
    if (posix_memalign(&ptr, 64, size) != 0) return NULL;
    return ptr;
}
static void free_aligned64(void* ptr) {
    free(ptr);
}
#endif

#if defined(__GNUC__) || defined(__clang__)
#define RESTRICT __restrict__
#else
#define RESTRICT
#endif

static void matrix_mult(int n) {
    size_t sz = (size_t)n * n * sizeof(double);
    double* RESTRICT a = (double*)alloc_aligned64(sz);
    double* RESTRICT b = (double*)alloc_aligned64(sz);
    double* RESTRICT c = (double*)alloc_aligned64(sz);

    for (int i = 0; i < n * n; i++) {
        a[i] = 1.0;
        b[i] = 2.0;
        c[i] = 0.0;
    }

    int BLOCK = 32;
    for (int i0 = 0; i0 < n; i0 += BLOCK) {
        int imax = (i0 + BLOCK < n) ? (i0 + BLOCK) : n;
        for (int k0 = 0; k0 < n; k0 += BLOCK) {
            int kmax = (k0 + BLOCK < n) ? (k0 + BLOCK) : n;
            for (int j0 = 0; j0 < n; j0 += BLOCK) {
                int jmax = (j0 + BLOCK < n) ? (j0 + BLOCK) : n;

                for (int i = i0; i < imax; i++) {
                    for (int k = k0; k < kmax; k++) {
                        double aik = a[i * n + k];
                        for (int j = j0; j < jmax; j++) {
                            c[i * n + j] += aik * b[k * n + j];
                        }
                    }
                }
            }
        }
    }

    free_aligned64(a);
    free_aligned64(b);
    free_aligned64(c);
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
