#!/usr/bin/env python3
import os
import sys
import subprocess

def run_cmd(cmd, cwd=None):
    print(f"[PGO Pipeline] Running: {cmd}")
    res = subprocess.run(cmd, shell=True, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        print(f"[Error] Command failed:\nSTDOUT:\n{res.stdout}\nSTDERR:\n{res.stderr}")
        return False, res.stdout, res.stderr
    return True, res.stdout, res.stderr

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    bench_src = os.path.join(script_dir, "bench_vit.c")
    gen_bin = os.path.join(script_dir, "bench_vit_pgo_gen")
    opt_bin = os.path.join(script_dir, "bench_vit_pgo_opt")
    std_bin = os.path.join(script_dir, "bench_vit_std")

    compiler = "gcc"
    
    print("=== Step 1: Standard -O3 Native Build ===")
    cmd_std = f"{compiler} -O3 -march=native -flto {bench_src} -o {std_bin}"
    ok, out, _ = run_cmd(cmd_std)
    if not ok:
        sys.exit(1)

    print("\n=== Step 2: PGO Instrument Build (-fprofile-generate) ===")
    cmd_gen = f"{compiler} -O3 -march=native -flto -fprofile-generate {bench_src} -o {gen_bin}"
    ok, out, _ = run_cmd(cmd_gen)
    if not ok:
        sys.exit(1)

    print("\n=== Step 3: Generating Profile Data ===")
    ok, profile_out, _ = run_cmd(f"{gen_bin}")
    print(profile_out)

    print("\n=== Step 4: PGO Optimized Recompile (-fprofile-use) ===")
    cmd_opt = f"{compiler} -O3 -march=native -flto -fprofile-use {bench_src} -o {opt_bin}"
    ok, out, _ = run_cmd(cmd_opt)
    if not ok:
        sys.exit(1)

    print("\n=== Step 5: Benchmark Comparison (Standard vs PGO Optimized) ===")
    print("\n--- Standard -O3 -march=native ---")
    _, std_out, _ = run_cmd(f"{std_bin}")
    print(std_out)

    print("--- PGO Optimized -O3 -march=native -fprofile-use ---")
    _, opt_out, _ = run_cmd(f"{opt_bin}")
    print(opt_out)

    print("✅ PGO Pipeline execution complete!")

if __name__ == "__main__":
    main()
