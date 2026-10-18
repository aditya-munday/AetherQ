#include "command_bench.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

#include <nlohmann/json.hpp>

#include "aetherq/kernels/kernels.hpp"
#include "aetherq/measurement/clock.hpp"
#include "aetherq/measurement/environment.hpp"
#include "aetherq/measurement/sampler.hpp"
#include "aetherq/runtime/config.hpp"
#include "aetherq/runtime/runtime.hpp"
#include "aetherq/workloads/allocation_stress.hpp"
#include "aetherq/workloads/workload.hpp"

#include "cli_common.hpp"

namespace aetherq::cli {

namespace {}

int command_bench_allocation(const BenchAllocationOptions& options) {
  const std::vector<AllocatorKind> allocators =
      options.allocator.empty()
          ? std::vector<AllocatorKind>{AllocatorKind::system, AllocatorKind::arena,
                                       AllocatorKind::pool}
          : std::vector<AllocatorKind>{};

  std::vector<AllocatorKind> selected = allocators;
  if (selected.empty()) {
    auto parsed = parse_allocator_name(options.allocator);
    if (!parsed) {
      print_error(parsed.error());
      return 2;
    }
    selected.push_back(parsed.value());
  }

  nlohmann::json rows = nlohmann::json::array();
  std::ostringstream text;
  text << "allocator,median_ms,p50_ns,p95_ns,p99_ns,allocations,reuse_count,peak_live_bytes,"
          "requested_bytes,bytes_per_item,items_per_second\n";

  bool ok = true;
  for (const AllocatorKind kind : selected) {
    CommonOptions common;
    common.allocator = std::string(to_string(kind));
    common.repetitions = options.repetitions;
    common.warmup = options.warmup;
    common.seed = options.seed;
    common.json_output = options.json_output;
    auto config = build_config(common);
    if (!config) {
      print_error(config.error());
      return 2;
    }
    auto runtime = Runtime::create(config.value(), default_logger());
    if (!runtime) {
      print_error(runtime.error());
      return 2;
    }

    AllocationRecorder recorder(true);
    AllocationStressResult outcome;
    auto body = [&] {
      recorder.reset();
      outcome = allocation_stress(*runtime.value(), options.items, kind, &recorder);
    };
    const SampleResult timing = collect_samples(config.value().measurement, body);
    const AllocationMetrics metrics = recorder.snapshot();
    const LatencySummary latency = summarize_latencies(metrics.latency_ns);
    const double median_ns = timing.stats.median;
    const double bytes_per_item =
        outcome.items == 0
            ? 0.0
            : static_cast<double>(metrics.requested_bytes) / static_cast<double>(outcome.items);

    if (outcome.checksum == 0.0 || outcome.allocations == 0) {
      ok = false;
    }

    text << to_string(kind) << ',' << median_ns * 1e-6 << ',' << latency.p50_ns << ','
         << latency.p95_ns << ',' << latency.p99_ns << ',' << metrics.allocation_count << ','
         << metrics.reuse_count << ',' << metrics.peak_live_bytes << ',' << metrics.requested_bytes
         << ',' << bytes_per_item << ','
         << (median_ns > 0.0 ? static_cast<double>(outcome.items) / (median_ns * 1e-9) : 0.0)
         << '\n';

    nlohmann::json row;
    row["allocator"] = std::string(to_string(kind));
    row["items"] = outcome.items;
    row["allocations"] = metrics.allocation_count;
    row["reuse_count"] = metrics.reuse_count;
    row["peak_live_bytes"] = metrics.peak_live_bytes;
    row["requested_bytes"] = metrics.requested_bytes;
    row["bytes_per_item"] = bytes_per_item;
    row["median_ms"] = median_ns * 1e-6;
    row["p50_ns"] = latency.p50_ns;
    row["p95_ns"] = latency.p95_ns;
    row["p99_ns"] = latency.p99_ns;
    row["checksum"] = outcome.checksum;
    rows.push_back(row);
  }

  if (options.json_output) {
    nlohmann::json document;
    document["items"] = options.items;
    document["results"] = rows;
    std::cout << document.dump(2) << '\n';
  } else {
    std::cout << text.str();
  }
  return ok ? 0 : 3;
}

int command_bench_scaling(const BenchScalingOptions& options) {
  std::ostringstream text;
  nlohmann::json rows = nlohmann::json::array();
  double baseline_median = 0.0;
  for (const std::size_t threads : options.thread_counts) {
    CommonOptions common;
    common.threads = threads;
    common.backend = options.backend;
    common.json_output = options.json_output;
    common.repetitions = options.repetitions;
    common.warmup = options.warmup;
    common.seed = options.seed;
    auto config = build_config(common);
    if (!config) {
      print_error(config.error());
      return 2;
    }
    Logger& logger = default_logger();
    auto runtime = Runtime::create(config.value(), logger);
    if (!runtime) {
      print_error(runtime.error());
      return 2;
    }
    auto workload = make_workload(options.workload);
    if (!workload) {
      print_error(workload.error());
      return 2;
    }
    WorkloadInputs inputs;
    inputs.paths = options.paths;
    inputs.options = options.options;
    inputs.assets = options.assets;
    inputs.observations = options.observations;
    RunReport report;
    auto executed = workload.value()->run(*runtime.value(), inputs, report);
    if (!executed) {
      print_error(executed.error());
      return 3;
    }
    const double median_ms = report.timing.stats.median * 1e-6;
    if (baseline_median == 0.0) {
      baseline_median = median_ms;
    }
    const double speedup = median_ms > 0.0 ? baseline_median / median_ms : 0.0;
    const double efficiency = speedup / static_cast<double>(threads);
    nlohmann::json row;
    row["threads"] = threads;
    row["median_ms"] = median_ms;
    row["speedup"] = speedup;
    row["efficiency"] = efficiency;
    rows.push_back(row);
    text << "threads=" << threads << " median_ms=" << median_ms << " speedup=" << speedup
         << " efficiency=" << efficiency << '\n';
  }
  if (options.json_output) {
    nlohmann::json document;
    document["workload"] = options.workload;
    document["rows"] = rows;
    std::cout << document.dump(2) << '\n';
    return 0;
  }
  std::cout << text.str();
  return 0;
}

int command_bench_kernels(const BenchKernelsOptions& options) {
  std::vector<std::size_t> sizes{64, 256, 1024, 4096, 16384};
  nlohmann::json document;
  document["filter"] = options.filter;
  nlohmann::json rows = nlohmann::json::array();
  for (const std::size_t size : sizes) {
    std::vector<double> values(size, 0.0);
    for (std::size_t i = 0; i < size; ++i) {
      values[i] = static_cast<double>(i) * 0.001;
    }
    const std::uint64_t start_scalar = SteadyClock::now_ns();
    const double scalar_total = scalar_sum(values);
    const std::uint64_t end_scalar = SteadyClock::now_ns();
    const std::uint64_t start_optimized = SteadyClock::now_ns();
    const double optimized_total = optimized_sum(values);
    const std::uint64_t end_optimized = SteadyClock::now_ns();
    nlohmann::json row;
    row["size"] = size;
    row["scalar_ns"] = static_cast<double>(end_scalar - start_scalar);
    row["optimized_ns"] = static_cast<double>(end_optimized - start_optimized);
    row["scalar_value"] = scalar_total;
    row["optimized_value"] = optimized_total;
    rows.push_back(row);
  }
  document["rows"] = rows;
  if (options.json_output) {
    std::cout << document.dump(2) << '\n';
    return 0;
  }
  std::cout << "size,scalar_ns,optimized_ns\n";
  for (const auto& row : rows) {
    std::cout << row["size"].get<std::size_t>() << ',' << row["scalar_ns"].get<double>() << ','
              << row["optimized_ns"].get<double>() << '\n';
  }
  return 0;
}

}
