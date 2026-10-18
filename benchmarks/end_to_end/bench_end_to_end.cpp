#include <chrono>
#include <cstdint>
#include <memory>

#include <benchmark/benchmark.h>

#include "aetherq/runtime/runtime.hpp"
#include "aetherq/workloads/monte_carlo.hpp"

using namespace aetherq;

namespace {

std::unique_ptr<Runtime> make_runtime(std::size_t threads, Backend backend) {
  RuntimeConfig config = default_config();
  config.threads = threads;
  config.backend = backend;
  config.scheduler = threads > 1 ? SchedulerPolicy::thread_pool : SchedulerPolicy::serial;
  auto runtime = Runtime::create(config, default_logger());
  return runtime.ok() ? std::move(runtime.value()) : nullptr;
}

void run_monte_carlo(benchmark::State& state) {
  const auto threads = static_cast<std::size_t>(state.range(0));
  auto runtime = make_runtime(threads, Backend::optimized);
  if (runtime == nullptr) {
    state.SkipWithError("runtime construction failed");
    return;
  }
  double sink = 0.0;
  for (auto _ : state) {
    const auto start = std::chrono::steady_clock::now();
    auto result = monte_carlo_price(*runtime, 200000, 64, 100.0, 100.0, 0.05, 0.2, 1.0, 20260101);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    if (!result.ok()) {
      state.SkipWithError("pricing failed");
      return;
    }
    sink += result.value().european_price;
    state.SetIterationTime(std::chrono::duration<double>(elapsed).count());
  }
  benchmark::DoNotOptimize(sink);
  state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * 200000);
}

}

BENCHMARK(run_monte_carlo)->Arg(1)->Arg(2)->Arg(4)->UseManualTime()->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
