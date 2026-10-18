#include "aetherq/memory/allocation_metrics.hpp"

#include <algorithm>
#include <cmath>

namespace aetherq {

AllocationRecorder::AllocationRecorder(bool record_latency) : record_latency_(record_latency) {}

void AllocationRecorder::record_allocation(std::size_t bytes, std::uint64_t latency_ns) {
  const std::lock_guard<std::mutex> lock(mutex_);
  metrics_.requested_bytes += bytes;
  metrics_.live_bytes += bytes;
  metrics_.allocation_count += 1;
  if (metrics_.live_bytes > metrics_.peak_live_bytes) {
    metrics_.peak_live_bytes = metrics_.live_bytes;
  }
  if (record_latency_) {
    metrics_.latency_ns.push_back(latency_ns);
  }
}

void AllocationRecorder::record_deallocation(std::size_t bytes) {
  const std::lock_guard<std::mutex> lock(mutex_);
  metrics_.deallocation_count += 1;
  if (bytes > metrics_.live_bytes) {
    metrics_.live_bytes = 0;
  } else {
    metrics_.live_bytes -= bytes;
  }
}

void AllocationRecorder::record_reuse() {
  const std::lock_guard<std::mutex> lock(mutex_);
  metrics_.reuse_count += 1;
}

void AllocationRecorder::record_failure() {
  const std::lock_guard<std::mutex> lock(mutex_);
  metrics_.failure_count += 1;
}

AllocationMetrics AllocationRecorder::snapshot() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return metrics_;
}

void AllocationRecorder::reset() {
  const std::lock_guard<std::mutex> lock(mutex_);
  metrics_ = AllocationMetrics{};
}

LatencySummary summarize_latencies(std::vector<std::uint64_t> samples) {
  LatencySummary summary;
  if (samples.empty()) {
    return summary;
  }
  std::sort(samples.begin(), samples.end());
  double total = 0.0;
  for (const auto value : samples) {
    total += static_cast<double>(value);
  }
  summary.mean_ns = total / static_cast<double>(samples.size());
  summary.max_ns = static_cast<double>(samples.back());
  const auto at = [&samples](double quantile) {
    const double index = quantile * static_cast<double>(samples.size() - 1);
    const auto lower = static_cast<std::size_t>(index);
    const auto upper = std::min(lower + 1, samples.size() - 1);
    const double fraction = index - static_cast<double>(lower);
    const double low = static_cast<double>(samples[lower]);
    const double high = static_cast<double>(samples[upper]);
    return low + ((high - low) * fraction);
  };
  summary.p50_ns = at(0.50);
  summary.p95_ns = at(0.95);
  summary.p99_ns = at(0.99);
  return summary;
}

}
