#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace aetherq {

struct SummaryStats {
  std::size_t sample_count{0};
  double mean{0.0};
  double stddev{0.0};
  double median{0.0};
  double mad{0.0};
  double min{0.0};
  double max{0.0};
  std::optional<double> p50;
  std::optional<double> p95;
  std::optional<double> p99;
  std::optional<double> p999;
};

SummaryStats summarize(std::span<const double> samples);
std::optional<double> percentile(std::span<const double> sorted_samples, double quantile);
double coefficient_of_variation(const SummaryStats& stats) noexcept;
bool distributions_overlap(const SummaryStats& lhs, const SummaryStats& rhs) noexcept;

}
