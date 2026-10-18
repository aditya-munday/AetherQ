#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "aetherq/measurement/stats.hpp"
#include "aetherq/runtime/config.hpp"

namespace aetherq {

struct SampleResult {
  std::vector<double> samples_ns;
  SummaryStats stats;
  std::size_t warmup_iterations{0};
  std::size_t repetitions{0};
};

SampleResult collect_samples(const MeasurementConfig& config, const std::function<void()>& body);

}
