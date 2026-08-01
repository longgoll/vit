use std::time::Instant;

fn fib(n: u64) -> u64 {
    if n <= 1 { return n; }
    fib(n - 1) + fib(n - 2)
}

fn matrix_mult(n: usize) {
    let a = vec![vec![1.0f64; n]; n];
    let b = vec![vec![2.0f64; n]; n];
    let mut c = vec![vec![0.0f64; n]; n];

    for i in 0..n {
        for k in 0..n {
            for j in 0..n {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

fn main() {
    let t1 = Instant::now();
    let f = fib(42);
    let fib_ms = t1.elapsed().as_secs_f64() * 1000.0;

    let t2 = Instant::now();
    matrix_mult(500);
    let mat_ms = t2.elapsed().as_secs_f64() * 1000.0;

    println!("Fibonacci(42): {} in {:.2} ms", f, fib_ms);
    println!("Matrix 500x500: in {:.2} ms", mat_ms);
}
