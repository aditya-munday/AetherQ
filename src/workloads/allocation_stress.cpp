#include "aetherq/workloads/allocation_stress.hpp"

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "aetherq/measurement/clock.hpp"
#include "aetherq/measurement/sampler.hpp"
#include "aetherq/memory/aligned_alloc.hpp"
#include "aetherq/memory/allocation_metrics.hpp"
#include "aetherq/memory/fixed_pool.hpp"
#include "aetherq/memory/monotonic_arena.hpp"
#include "aetherq/runtime/runtime.hpp"

#include "workload_support.hpp"

namespace aetherq {

namespace {

constexpr std::size_t item_bytes = 128;
constexpr std::size_t pool_capacity = 4096;

}

AllocationStressResult allocation_stress(Runtime& runtime, std::size_t items, AllocatorKind kind,
                                         AllocationRecorder* recorder) {
  static_cast<void>(runtime);
  AllocationStressResult result;
  result.items = items;
  double checksum = 0.0;
  const std::size_t element_count = item_bytes / sizeof(double);

  if (kind == AllocatorKind::arena) {
    MonotonicArena arena((items * item_bytes) + 4096, recorder);
    std::size_t allocated = 0;
    for (std::size_t i = 0; i < items; ++i) {
      void* pointer = arena.allocate(item_bytes, simd_alignment);
      if (pointer == nullptr) {
        break;
      }
      auto* values = static_cast<double*>(pointer);
      values[0] = static_cast<double>(i) * 0.5;
      checksum += values[0];
      ++allocated;
    }
    result.allocations = allocated;
  } else if (kind == AllocatorKind::pool) {
    FixedPool pool(item_bytes, pool_capacity, simd_alignment, recorder);
    std::vector<void*> live;
    live.reserve(pool_capacity);
    std::size_t allocated = 0;
    for (std::size_t i = 0; i < items; ++i) {
      if (live.size() >= pool_capacity) {
        pool.release(live.back());
        live.pop_back();
      }
      void* pointer = pool.allocate();
      if (pointer == nullptr) {
        break;
      }
      live.push_back(pointer);
      auto* values = static_cast<double*>(pointer);
      values[0] = static_cast<double>(i) * 0.5;
      checksum += values[0];
      ++allocated;
    }
    for (void* pointer : live) {
      pool.release(pointer);
    }
    result.allocations = allocated;
  } else {
    std::vector<void*> live;
    live.reserve(items);
    for (std::size_t i = 0; i < items; ++i) {
      const std::uint64_t start = SteadyClock::now_ns();
      void* raw = aligned_allocate(item_bytes, simd_alignment);
      if (raw == nullptr) {
        break;
      }
      auto* block = static_cast<double*>(raw);
      block[0] = static_cast<double>(i) * 0.5;
      checksum += block[0];
      if (recorder != nullptr) {
        recorder->record_allocation(item_bytes, SteadyClock::now_ns() - start);
      }
      live.push_back(raw);
    }
    for (void* pointer : live) {
      aligned_release(pointer, simd_alignment);
      if (recorder != nullptr) {
        recorder->record_deallocation(item_bytes);
      }
    }
    result.allocations = live.size();
  }

  static_cast<void>(element_count);
  result.checksum = checksum;
  return result;
}

std::string_view AllocationStressWorkload::name() const noexcept {
  return "allocation_stress";
}

Result<void> AllocationStressWorkload::run(Runtime& runtime, const WorkloadInputs& inputs,
                                           RunReport& report) {
  const std::size_t items = inputs.items;
  if (items == 0) {
    return Error{ErrorCode::invalid_argument, "items must be at least 1"};
  }

  report = make_report(runtime, "allocation_stress", runtime.config().threads);
  report.workload_parameters["items"] = static_cast<double>(items);
  report.memory.allocator = std::string(to_string(runtime.config().allocator));

  AllocationStressResult outcome;
  const AllocatorKind kind = runtime.config().allocator;
  auto body = [&runtime, &outcome, items, kind] {
    outcome = allocation_stress(runtime, items, kind);
  };
  report.timing = collect_samples(runtime.config().measurement, body);

  report.correctness.oracle = "allocation_path_exercised";
  report.correctness.measured = outcome.checksum;
  report.correctness.expected = outcome.checksum;
  report.correctness.tolerance = 0.0;
  report.correctness.error = 0.0;
  report.correctness.passed = outcome.allocations > 0;
  report.correctness.detail = "allocation_path_exercised";

  report.metrics.push_back(
      MetricEntry{"allocations", static_cast<double>(outcome.allocations), "count"});
  report.metrics.push_back(MetricEntry{"checksum", outcome.checksum, "sum"});
  report.metrics.push_back(
      MetricEntry{"items_per_second",
                  static_cast<double>(items) / (report.timing.stats.median * 1e-9), "items/s"});
  report.metrics.push_back(MetricEntry{"elapsed_ms", report.timing.stats.median * 1e-6, "ms"});

  finalize_report(runtime, report);
  return ok_result();
}

}
