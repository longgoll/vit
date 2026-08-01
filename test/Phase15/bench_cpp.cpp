#include <iostream>
#include <vector>
#include <chrono>

long long fib(long long n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

void matrix_mult(int n) {
    std::vector<std::vector<double>> a(n, std::vector<double>(n, 1.0));
    std::vector<std::vector<double>> b(n, std::vector<double>(n, 2.0));
    std::vector<std::vector<double>> c(n, std::vector<double>(n, 0.0));

    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            for (int j = 0; j < n; j++) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

int main() {
    auto t1 = std::chrono::high_resolution_clock::now();
    long long f = fib(42);
    auto t2 = std::chrono::high_resolution_clock::now();
    double fib_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();

    t1 = std::chrono::high_resolution_clock::now();
    matrix_mult(500);
    t2 = std::chrono::high_resolution_clock::now();
    double mat_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();

    std::cout << "Fibonacci(42): " << f << " in " << fib_ms << " ms\n";
    std::cout << "Matrix 500x500: in " << mat_ms << " ms\n";
    return 0;
}
