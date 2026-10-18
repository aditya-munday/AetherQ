#include "aetherq/measurement/clock.hpp"

#include <chrono>

namespace aetherq {

std::uint64_t SteadyClock::now_ns() noexcept {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

double SteadyClock::now_seconds() noexcept {
  return static_cast<double>(now_ns()) * 1e-9;
}

}
