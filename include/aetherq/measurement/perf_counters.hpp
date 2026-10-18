#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace aetherq {

struct HardwareCounters {
  bool available{false};
  std::string unavailable_reason;
  std::optional<std::uint64_t> cycles;
  std::optional<std::uint64_t> instructions;
  std::optional<std::uint64_t> cache_misses;
  std::optional<std::uint64_t> branch_misses;
  std::optional<double> ipc;
};

HardwareCounters probe_hardware_counters();
bool hardware_counters_supported() noexcept;

}
