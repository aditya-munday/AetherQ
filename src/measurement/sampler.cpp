#include "aetherq/measurement/sampler.hpp"

#include "aetherq/measurement/clock.hpp"

namespace aetherq {

SampleResult collect_samples(const MeasurementConfig& config, const std::function<void()>& body) {
  SampleResult result;
  result.warmup_iterations = config.warmup_iterations;
  result.repetitions = config.repetitions;
  for (std::size_t i = 0; i < config.warmup_iterations; ++i) {
    body();
  }
  result.samples_ns.reserve(config.repetitions);
  for (std::size_t i = 0; i < config.repetitions; ++i) {
    const std::uint64_t start = SteadyClock::now_ns();
    body();
    const std::uint64_t end = SteadyClock::now_ns();
    result.samples_ns.push_back(static_cast<double>(end - start));
  }
  result.stats = summarize(result.samples_ns);
  return result;
}

}
