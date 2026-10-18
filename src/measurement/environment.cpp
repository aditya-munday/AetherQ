#include "aetherq/measurement/environment.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#if defined(__linux__)
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

#include "aetherq/runtime/version.hpp"

namespace aetherq {

namespace {

std::string read_first_line(const std::string& path) {
  std::ifstream stream(path);
  if (!stream) {
    return {};
  }
  std::string line;
  std::getline(stream, line);
  return line;
}

std::string read_file(const std::string& path) {
  std::ifstream stream(path);
  if (!stream) {
    return {};
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

std::string cpu_model() {
#if defined(__linux__)
  std::ifstream stream("/proc/cpuinfo");
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind("model name", 0) == 0) {
      const auto colon = line.find(':');
      if (colon != std::string::npos) {
        std::string value = line.substr(colon + 1);
        const auto start = value.find_first_not_of(" \t");
        return start == std::string::npos ? std::string{} : value.substr(start);
      }
    }
  }
#endif
  return "unknown";
}

std::size_t physical_cores() {
#if defined(__linux__)
  const std::string text = read_file("/proc/cpuinfo");
  std::istringstream stream(text);
  std::string line;
  std::size_t unique = 0;
  std::size_t last_id = static_cast<std::size_t>(-1);
  bool saw = false;
  while (std::getline(stream, line)) {
    if (line.rfind("physical id", 0) == 0) {
      const auto colon = line.find(':');
      if (colon != std::string::npos) {
        const auto id = static_cast<std::size_t>(std::stoul(line.substr(colon + 1)));
        if (!saw || id != last_id) {
          ++unique;
          last_id = id;
          saw = true;
        }
      }
    }
  }
  if (unique > 0) {
    return unique;
  }
#endif
  return std::thread::hardware_concurrency();
}

std::size_t cache_size(const char* index) {
#if defined(__linux__)
  const std::string base =
      std::string("/sys/devices/system/cpu/cpu0/cache/index") + index + "/size";
  const std::string value = read_first_line(base);
  if (value.empty()) {
    return 0;
  }
  std::size_t multiplier = 1;
  std::string digits = value;
  if (!value.empty()) {
    const char suffix = value.back();
    if (suffix == 'K' || suffix == 'k') {
      multiplier = 1024;
      digits = value.substr(0, value.size() - 1);
    } else if (suffix == 'M' || suffix == 'm') {
      multiplier = 1024 * 1024;
      digits = value.substr(0, value.size() - 1);
    }
  }
  try {
    return static_cast<std::size_t>(std::stoul(digits)) * multiplier;
  } catch (...) {
    return 0;
  }
#else
  static_cast<void>(index);
  return 0;
#endif
}

std::string frequency_governor() {
#if defined(__linux__)
  const std::string value =
      read_first_line("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
  return value.empty() ? std::string("unavailable") : value;
#else
  return "unavailable";
#endif
}

std::string utc_timestamp() {
  const std::time_t now = std::time(nullptr);
  std::tm tm_buf{};
#if defined(_WIN32)
  gmtime_s(&tm_buf, &now);
#else
  gmtime_r(&now, &tm_buf);
#endif
  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
  return buffer;
}

}

EnvironmentInfo capture_environment() {
  EnvironmentInfo info;
  info.cpu_model = cpu_model();
  info.logical_cores = std::thread::hardware_concurrency();
  info.physical_cores = physical_cores();
  info.l1_cache_bytes = cache_size("0");
  info.l2_cache_bytes = cache_size("2");
  info.l3_cache_bytes = cache_size("3");
  info.frequency_governor = frequency_governor();
  info.compiler = std::string(compiler_id());
  info.compiler_version = std::string(compiler_version());
  info.build_type = std::string(build_type());
  info.git_commit = std::string(git_commit());
  info.git_dirty = std::string(git_dirty());
  info.timestamp_utc = utc_timestamp();
#if defined(__linux__)
  struct utsname uts{};
  if (uname(&uts) == 0) {
    info.os_name = uts.sysname;
    info.kernel_version = uts.release;
  }
#endif
  if (info.os_name.empty()) {
    info.os_name = "unknown";
  }
  return info;
}

std::size_t peak_rss_bytes() {
#if defined(__linux__)
  struct rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    return static_cast<std::size_t>(usage.ru_maxrss) * 1024;
  }
#endif
  return 0;
}

std::size_t current_rss_bytes() {
#if defined(__linux__)
  const std::string text = read_file("/proc/self/statm");
  std::istringstream stream(text);
  std::size_t total_pages = 0;
  std::size_t resident_pages = 0;
  if (stream >> total_pages >> resident_pages) {
    static_cast<void>(total_pages);
    return resident_pages * static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
  }
#endif
  return 0;
}

}
