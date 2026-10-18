#include "aetherq/measurement/perf_counters.hpp"

#include <cstdint>
#include <fstream>
#include <string>

#if defined(__linux__)
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace aetherq {

namespace {

std::string perf_paranoid_value() {
#if defined(__linux__)
  std::ifstream stream("/proc/sys/kernel/perf_event_paranoid");
  std::string value;
  if (stream >> value) {
    return value;
  }
#endif
  return {};
}

}

bool hardware_counters_supported() noexcept {
#if defined(__linux__)
  const std::string paranoid = perf_paranoid_value();
  if (paranoid.empty()) {
    return false;
  }
  try {
    return std::stoi(paranoid) < 2;
  } catch (...) {
    return false;
  }
#else
  return false;
#endif
}

HardwareCounters probe_hardware_counters() {
  HardwareCounters counters;
#if defined(__linux__)
  const std::string paranoid = perf_paranoid_value();
  if (paranoid.empty()) {
    counters.unavailable_reason = "perf_event_paranoid not readable";
    return counters;
  }
  int level = 0;
  try {
    level = std::stoi(paranoid);
  } catch (...) {
    counters.unavailable_reason = "perf_event_paranoid unparseable: " + paranoid;
    return counters;
  }
  if (level >= 2) {
    counters.unavailable_reason =
        "perf_event_paranoid=" + paranoid + " denies unprivileged hardware counter access";
    return counters;
  }
  counters.unavailable_reason =
      "hardware counter collection not implemented for this platform build";
  return counters;
#else
  counters.unavailable_reason = "perf_event_open unsupported on this platform";
  return counters;
#endif
}

}
