# HKD Kernel macOS v1 — Reproducible Benchmark Edition

HKD Kernel is a native C library for exact sparse/incremental computation. It does **not** replace XNU, modify CPU microcode, disable SIP, or change Apple ALU hardware.

## One-command macOS benchmark

Requires Xcode Command Line Tools (`clang`/`make`).

```sh
./run_all_benchmarks.sh
```

The runner builds from source and writes a timestamped result file under `benchmark/`.

## Three benchmark classes

1. **Dense arithmetic control (`bench_dense`)** — both implementations perform the same full-array summation. A huge speedup is *not* expected. This is the control against misleading CPU-speed claims.
2. **Sparse incremental aggregate (`bench_native`)** — baseline rescans N values after every mutation; HKD updates a materialized exact aggregate from the changed leaf only.
3. **Application-style portfolio refresh (`bench_app`)** — baseline fully revalues all instrument exposures after each price change; HKD changes only the affected weighted contribution. A fresh full revaluation verifies the final answer exactly.

All integer arithmetic is verified for exact final-state equality.

## Tunable sizes

```sh
HKD_DENSE_N=8000000 \
HKD_DENSE_REPS=40 \
HKD_SPARSE_N=4000000 \
HKD_SPARSE_UPDATES=200 \
HKD_APP_N=2000000 \
HKD_APP_UPDATES=300 \
./run_all_benchmarks.sh
```

For a heavier Apple Silicon run, increase N and update counts while keeping memory pressure reasonable.

## Individual commands

```sh
make
./bench_dense 8000000 40
./bench_native 4000000 200
./bench_app 2000000 300
```

## Interpretation

A large sparse/application speedup demonstrates reduced logical work for workloads that support incremental state. It does not mean arbitrary instructions or arbitrary Mac applications execute by that multiplier. The dense control is specifically included to make that distinction testable.

`TESTED_HOST_RESULTS.txt` contains the pre-packaging validation from the build environment. Those measurements are Linux/x86-64 and are **not** presented as Apple Silicon measurements. Run `./run_all_benchmarks.sh` on the Mac to obtain the Apple Silicon numbers and hardware profile.
