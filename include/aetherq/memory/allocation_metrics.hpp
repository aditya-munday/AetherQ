#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace aetherq {

struct LatencySummary {
  double mean_ns{0.0};
  double p50_ns{0.0};
  double p95_ns{0.0};
  double p99_ns{0.0};
  double max_ns{0.0};
};

struct AllocationMetrics {
  std::size_t requested_bytes{0};
  std::size_t peak_live_bytes{0};
  std::size_t live_bytes{0};
  std::uint64_t allocation_count{0};
  std::uint64_t deallocation_count{0};
  std::uint64_t reuse_count{0};
  std::uint64_t failure_count{0};
  std::vector<std::uint64_t> latency_ns{};
};

class AllocationRecorder {
public:
  explicit AllocationRecorder(bool record_latency);

  void record_allocation(std::size_t bytes, std::uint64_t latency_ns);
  void record_deallocation(std::size_t bytes);
  void record_reuse();
  void record_failure();

  [[nodiscard]] AllocationMetrics snapshot() const;
  void reset();

private:
  mutable std::mutex mutex_;
  AllocationMetrics metrics_;
  bool record_latency_;
};

LatencySummary summarize_latencies(std::vector<std::uint64_t> samples);

}
