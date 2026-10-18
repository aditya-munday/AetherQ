# AetherQ

AetherQ is a native C++20 runtime and CLI for reproducible, hardware-aware quantitative
workloads. A single library plus one `aetherq` executable run dense numerical kernels,
Monte Carlo pricing, Black-Scholes batch pricing with Greeks, factor/portfolio risk, and
allocation-heavy task batches under controlled execution policies. Every optimized path
ships alongside a scalar reference that acts as the correctness oracle, and every run
emits a versioned JSON report carrying environment metadata, timing distributions,
memory metrics, and correctness evidence.

Non-goals: AetherQ is not a trading bot, not an order book, not a web application, and
not an exchange-connected system. It has no GPU/CUDA backend, no Python bindings, and no
graphical dashboard in this version. It is fully useful headless.

## Architecture

```
CLI (aetherq)
  |
  v
Workload API  (monte_carlo | black_scholes | factor_risk | allocation_stress)
  |
  v
ExecutionPolicy  (serial | thread_pool, parallel_for chunking)
  |
  +--> Scheduler (SerialExecutor | ThreadPoolExecutor, bounded queue)
  +--> Memory    (system | MonotonicArena | FixedPool, aligned + pmr adapters)
  |
  v
CPU Kernels  (scalar reference | optimized, feature-dispatched)
  |
  v
Measurement  (clock, sampler, stats, environment, perf counters)
  |
  v
JSON / CSV  ->  report | compare | tools/plot_results.py
```

Data flow: `CLI -> Workload API -> ExecutionPolicy -> Scheduler + Memory -> CPU Kernels
-> Measurement -> JSON/CSV -> Plots`.

Module responsibilities:

- `runtime/` - `RuntimeConfig` (threads, scheduler, allocator, backend, seed, measurement
  settings), versioned JSON config parsing with strict validation, `Result<T>` error model,
  logging, and `Runtime` which owns the executor and logger.
- `memory/` - aligned allocation, RAII `Buffer<T>`, `MonotonicArena`, `FixedPool`,
  `std::pmr` resource adapters, and `AllocationRecorder` metrics (requested/peak/live bytes,
  allocation and reuse counts, latency samples).
- `executor/` - `Executor` interface with task batches, first-error propagation, cooperative
  cancellation, and metrics; `SerialExecutor` reference; `ThreadPoolExecutor` with a bounded
  queue, backpressure, safe shutdown, and `parallel_for` (static, guided, fixed-grain).
- `kernels/` - scalar reference and optimized kernels behind one interface, CPU feature
  detection (SSE4.2/AVX2/AVX-512/NEON) with safe fallback, and counter-based Philox RNG.
- `workloads/` - the four workloads above; each declares inputs, a correctness oracle, and
  workload-specific metrics.
- `measurement/` - steady clock, sampler with warmup/repetitions, robust statistics,
  environment capture, perf-counter probing, and the report schema with validation, CSV, and
  comparison.

## Execution policies and memory strategies

Execution:

- `serial` runs every task on the calling thread. This is the reference policy.
- `thread_pool` runs independent batches over a fixed worker pool with a bounded queue
  (capacity 256) so submissions apply backpressure instead of growing without bound.
- `parallel_for` partitions index ranges with `static_chunk`, `guided`, or `fixed_grain`
  policies; `chunk_grain` controls the split.

Memory:

- `system` uses aligned `operator`-backed allocation.
- `arena` (`MonotonicArena`) bumps a pointer through a preallocated block and resets as a
  phase; deallocation is a no-op.
- `pool` (`FixedPool`) recycles fixed-size slots through a free list.

Allocator comparison, 100000 items x 128 bytes, 7 repetitions after 2 warmup runs, measured
on the machine below (`aetherq bench allocation --items 100000 --repetitions 7 --warmup 2`):

| allocator | median ms | p50 ns | p95 ns | p99 ns | peak live bytes | reuse count |
|-----------|-----------|--------|--------|--------|-----------------|-------------|
| system    | 22.17     | 50     | 110    | 2020   | 12800000        | 0           |
| arena     | 6.82      | 30     | 30     | 40     | 12800000        | 0           |
| pool      | 6.77      | 30     | 40     | 40     | 1048576         | 100000      |

The arena and pool are about 3.3x faster than the system allocator here, and the pool holds
peak live bytes about 12x lower because it recycles a bounded slot set. The system path's
p99 tail (2020 ns) comes from the allocator occasionally hitting the kernel; arena and pool
tails stay flat.

Thread scaling, Monte Carlo 400000 paths, 5 repetitions after 2 warmup runs
(`aetherq bench scaling --workload monte_carlo --paths 400000 --threads 1,2,4`):

| threads | median ms | speedup | efficiency |
|---------|-----------|---------|------------|
| 1       | 733.27    | 1.00    | 1.00       |
| 2       | 367.65    | 1.99    | 1.00       |
| 4       | 240.06    | 3.05    | 0.76       |

Efficiency falls at four threads because the host exposes four logical cores on one physical
core (see environment), so the last two workers contend for the same execution units.

Black-Scholes scaling, 200000 options: 1 thread 25.17 ms, 2 threads 13.64 ms (1.85x, 0.92
efficiency), 4 threads 10.87 ms (2.32x, 0.58 efficiency).

Factor-risk scaling, 256 assets x 2000 observations: 1 thread 442.77 ms, 2 threads 445.91 ms
(0.99x), 4 threads 453.64 ms (0.98x). The factor-risk workload does not scale on this host;
the matrix work is small relative to per-task overhead and the pool adds scheduling cost.
This is reported as a limitation, not a win.

## Correctness model

Every numerical operation has a scalar reference kernel and an optimized kernel behind one
interface. The backend is selected by config plus runtime CPU feature detection, and the
scalar path remains the oracle.

- Monte Carlo validates the European price against the Black-Scholes closed form using a
  six-standard-error confidence band derived from the measured standard error, not a magic
  constant. The Asian option is priced alongside it.
- Black-Scholes validates price and all Greeks against a high-precision scalar reference and
  checks put-call parity; the reported `max_parity_error` was 4.3e-14.
- Factor risk validates portfolio variance against a scalar reference (tolerance 1e-12) and
  checks that component risk sums to total volatility.
- Optimized kernels are tested against scalar references across empty, size-1, non-vector-width,
  unaligned, denormal, large-magnitude, and NaN/Inf cases with per-kernel relative tolerances.

Reproducibility: the counter-based RNG partitions by block index, so Monte Carlo results are
bit-identical across thread counts. Verified on the machine below: 100000 paths with seed
20260101 produced `10.500916845602` at both 1 and 4 threads. Tolerance-bounded comparisons
apply only to the optimized-vs-scalar kernel checks and the factor-risk covariance path.

## Results schema overview

Reports are JSON with `schema_version` (currently 1), validated on read by
`aetherq validate`. Top-level fields: `run_id`, `workload`, `backend`, `seed`, `environment`
(CPU model, logical/physical cores, cache sizes, governor, OS/kernel, compiler and version,
build type, git commit and dirty flag, timestamp), `config` (the full `RuntimeConfig`),
`execution` (scheduler, threads, tasks submitted/completed, queue high-water mark, parallel
efficiency), `memory` (allocation/deallocation/reuse counts, requested/peak live bytes, peak
RSS, allocator), `counters` (hardware counters or an `unavailable_reason`), `timing`
(samples, statistics, repetitions), `workload_parameters`, `metrics` (workload-specific
name/value/unit entries), and `correctness` (oracle, measured, expected, tolerance, error,
pass/fail). Hardware counters that cannot be read are reported as unavailable with the
reason and are never zero-filled.

## Build, test, benchmark, reproduce

Prerequisites: CMake >= 3.25, Ninja, a C++20 compiler (GCC 13+ or Clang 17+), and vcpkg with
`VCPKG_ROOT` set. On this machine GCC 14.2.0 and Clang 18 were used.

```sh
git clone https://github.com/aditya-munday/AetherQ
cd AetherQ
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset release
cmake --build --preset release -j
ctest --preset release --output-on-failure
```

Run the CLI:

```sh
./build/release/aetherq system-info
./build/release/aetherq run monte_carlo --paths 400000 --threads 4 --backend optimized \
  --repetitions 7 --warmup 2 --output results/mc.json --json
./build/release/aetherq run black_scholes --options 200000 --threads 4 --backend optimized
./build/release/aetherq run factor_risk --assets 256 --observations 2000 --threads 4
./build/release/aetherq run monte_carlo --config configs/monte_carlo.json --paths 400000
./build/release/aetherq bench allocation --items 100000 --repetitions 7 --warmup 2
./build/release/aetherq bench scaling --workload monte_carlo --threads 1,2,4 --paths 400000
./build/release/aetherq bench kernels
./build/release/aetherq validate results/mc.json
./build/release/aetherq report results/mc.json --format text
./build/release/aetherq compare results/scalar.json results/optimized.json
```

Sanitizers and coverage:

```sh
cmake --preset asan-ubsan && cmake --build --preset asan-ubsan -j && ctest --preset asan-ubsan
cmake --preset tsan && cmake --build --preset tsan -j
setarch $(uname -m) -R ctest --preset tsan
cmake --preset coverage && cmake --build --preset coverage -j && ctest --preset coverage
```

Plots and tooling (matplotlib + standard library only):

```sh
python3 tools/plot_results.py results/mc.json results/scalar.json --out results
python3 tools/compare_runs.py results/scalar.json results/optimized.json
python3 tools/check_policy.py
```

## Microbenchmarks and baselines

Kernel microbenchmarks (`benchmarks/micro`, Google Benchmark, 3 repetitions reported as
medians) show the optimized sum kernel 1.98x faster than scalar at 1M elements (118298 ns vs
233970 ns) and the optimized dot kernel 4.33x faster than scalar at 1M elements (216077 ns vs
935098 ns). Against Eigen on identical inputs, `aetherq_dot` reaches parity (203209 ns vs
206891 ns at 1M elements), while `aetherq_matrix_matrix` at size 512 is 1.83x slower than
Eigen (28081431 ns vs 15311821 ns); Eigen is the mature baseline and the blocked kernel does
not beat it at that size. This is reported honestly.

Compiler vectorization was confirmed rather than assumed: `g++ -O3 -fopt-info-vec-optimized`
on the scalar kernels reports vectorized loops using 32-byte and 64-byte vectors (AVX2 and
AVX-512), with alias-versioned loops where pointer aliasing was possible.

Environment for all numbers above: AMD EPYC 9B14, 4 logical / 1 physical cores, 32 KiB L1 /
1024 KiB L2 / 32 MiB L3, Linux 6.8.0-1055-gke, GCC 14.2.0, Release build. Hardware counters
were unavailable: `perf_event_paranoid=4` denies unprivileged access, so cycle, instruction,
IPC, cache-miss, and branch-miss fields are reported as unavailable with that reason.

## Supported platforms and known limitations

Tested: Linux x86_64 with GCC 13+/Clang 17+ (CI matrix runs GCC and Clang, Debug and
Release). SIMD dispatch covers SSE4.2, AVX2, and AVX-512 with a scalar fallback; NEON is
detected but not exercised on this hardware.

Known limitations:

- The factor-risk workload does not scale with threads on this host (0.98x at four threads);
  the matrix work is too small relative to pool overhead.
- The blocked matrix-matrix kernel is 1.83x slower than Eigen at size 512.
- The optimized backend shows no meaningful advantage over scalar for Black-Scholes and
  factor-risk end-to-end, where memory traffic and allocation dominate; only the reduction
  kernels (sum, dot) show a clear SIMD win.
- Hardware performance counters are unavailable on this host due to
  `perf_event_paranoid=4`; the report states this rather than zero-filling.
- TSan requires ASLR disabled (`setarch -R`) on kernel 6.8 because of the known
  "unexpected memory mapping" incompatibility; this is an environment constraint, not a
  code defect.
- No GPU backend, no Python bindings, and no work-stealing scheduler; work-stealing was not
  implemented because the profiling study did not show shared-queue contention on this host.
- The no-comments policy is enforced mechanically by `tools/check_policy.py`.
