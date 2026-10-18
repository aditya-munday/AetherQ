#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "aetherq/runtime/config.hpp"

namespace aetherq {

struct EnvironmentInfo {
  std::string cpu_model;
  std::size_t logical_cores{0};
  std::size_t physical_cores{0};
  std::size_t l1_cache_bytes{0};
  std::size_t l2_cache_bytes{0};
  std::size_t l3_cache_bytes{0};
  std::string frequency_governor;
  std::string os_name;
  std::string kernel_version;
  std::string compiler;
  std::string compiler_version;
  std::string build_type;
  std::string git_commit;
  std::string git_dirty;
  std::string timestamp_utc;
};

EnvironmentInfo capture_environment();

std::size_t peak_rss_bytes();
std::size_t current_rss_bytes();

}
