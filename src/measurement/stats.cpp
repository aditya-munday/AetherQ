#include "aetherq/measurement/stats.hpp"

#include <algorithm>
#include <cmath>

namespace aetherq {

std::optional<double> percentile(std::span<const double> sorted_samples, double quantile) {
  if (sorted_samples.empty()) {
    return std::nullopt;
  }
  if (quantile < 0.0 || quantile > 1.0) {
    return std::nullopt;
  }
  const double index = quantile * static_cast<double>(sorted_samples.size() - 1);
  const auto lower = static_cast<std::size_t>(index);
  const auto upper = std::min(lower + 1, sorted_samples.size() - 1);
  const double fraction = index - static_cast<double>(lower);
  const double low = sorted_samples[lower];
  const double high = sorted_samples[upper];
  return low + ((high - low) * fraction);
}

SummaryStats summarize(std::span<const double> samples) {
  SummaryStats stats;
  stats.sample_count = samples.size();
  std::vector<double> sorted(samples.begin(), samples.end());
  if (sorted.empty()) {
    return stats;
  }
  std::sort(sorted.begin(), sorted.end());

  double total = 0.0;
  for (const double value : sorted) {
    total += value;
  }
  stats.mean = total / static_cast<double>(sorted.size());
  stats.min = sorted.front();
  stats.max = sorted.back();

  double variance = 0.0;
  for (const double value : sorted) {
    const double delta = value - stats.mean;
    variance += delta * delta;
  }
  stats.stddev =
      sorted.size() > 1 ? std::sqrt(variance / static_cast<double>(sorted.size() - 1)) : 0.0;

  stats.median = percentile(sorted, 0.5).value();
  std::vector<double> deviations;
  deviations.reserve(sorted.size());
  for (const double value : sorted) {
    deviations.push_back(std::fabs(value - stats.median));
  }
  std::sort(deviations.begin(), deviations.end());
  stats.mad = percentile(deviations, 0.5).value();

  constexpr std::size_t percentile_threshold = 20;
  if (sorted.size() >= percentile_threshold) {
    stats.p50 = percentile(sorted, 0.50);
    stats.p95 = percentile(sorted, 0.95);
  }
  if (sorted.size() >= 100) {
    stats.p99 = percentile(sorted, 0.99);
  }
  if (sorted.size() >= 1000) {
    stats.p999 = percentile(sorted, 0.999);
  }
  return stats;
}

double coefficient_of_variation(const SummaryStats& stats) noexcept {
  if (stats.mean == 0.0) {
    return 0.0;
  }
  return stats.stddev / stats.mean;
}

bool distributions_overlap(const SummaryStats& lhs, const SummaryStats& rhs) noexcept {
  const double lhs_low = lhs.mean - lhs.stddev;
  const double lhs_high = lhs.mean + lhs.stddev;
  const double rhs_low = rhs.mean - rhs.stddev;
  const double rhs_high = rhs.mean + rhs.stddev;
  return lhs_low <= rhs_high && rhs_low <= lhs_high;
}

}
