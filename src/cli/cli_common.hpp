#pragma once

#include <string>

#include "aetherq/runtime/config.hpp"
#include "aetherq/runtime/result.hpp"

namespace aetherq::cli {

struct CommonOptions {
  std::size_t threads{1};
  std::string backend{"scalar"};
  std::string allocator{"system"};
  std::string config_path;
  std::size_t seed{20260101};
  bool json_output{false};
  bool correctness_mode{false};
  std::size_t repetitions{5};
  std::size_t warmup{1};
};

Result<RuntimeConfig> build_config(const CommonOptions& options);
std::string escape_csv(const std::string& value);
void print_error(const Error& error);

}
