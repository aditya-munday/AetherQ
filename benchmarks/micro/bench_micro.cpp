#include <cstddef>
#include <vector>

#include <benchmark/benchmark.h>

#include "aetherq/kernels/dispatch.hpp"
#include "aetherq/kernels/kernels.hpp"
#include "aetherq/kernels/rng.hpp"

using namespace aetherq;

namespace {

std::vector<double> make_values(std::size_t count) {
  std::vector<double> values(count, 0.0);
  Philox4x32::fill_uniform(1234, 1, 0, values);
  return values;
}

void run_sum(benchmark::State& state, double (*kernel)(std::span<const double>)) {
  const auto size = static_cast<std::size_t>(state.range(0));
  const std::vector<double> values = make_values(size);
  double sink = 0.0;
  for (auto _ : state) {
    sink += kernel(values);
  }
  benchmark::DoNotOptimize(sink);
  state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) *
                          static_cast<std::int64_t>(size));
}

void run_dot(benchmark::State& state,
             double (*kernel)(std::span<const double>, std::span<const double>)) {
  const auto size = static_cast<std::size_t>(state.range(0));
  const std::vector<double> lhs = make_values(size);
  const std::vector<double> rhs = make_values(size + 1);
  double sink = 0.0;
  for (auto _ : state) {
    sink += kernel(lhs, rhs);
  }
  benchmark::DoNotOptimize(sink);
}

}

BENCHMARK_CAPTURE(run_sum, scalar, &scalar_sum)->Range(64, 1 << 20);
BENCHMARK_CAPTURE(run_sum, optimized, &optimized_sum)->Range(64, 1 << 20);
BENCHMARK_CAPTURE(run_dot, scalar, &scalar_dot)->Range(64, 1 << 20);
BENCHMARK_CAPTURE(run_dot, optimized, &optimized_dot)->Range(64, 1 << 20);

BENCHMARK_MAIN();
