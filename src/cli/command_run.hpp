#pragma once

#include <cstddef>
#include <string>

#include "aetherq/runtime/result.hpp"

namespace aetherq::cli {

struct RunOptions {
  std::string workload;
  std::size_t paths{100000};
  std::size_t options{100000};
  std::size_t assets{32};
  std::size_t observations{1024};
  std::size_t items{100000};
  std::string output;
  bool json_output{false};
  std::size_t threads{1};
  std::string backend{"scalar"};
  std::string allocator{"system"};
  std::string config_path;
  std::size_t seed{20260101};
  bool correctness_mode{false};
  std::size_t repetitions{5};
  std::size_t warmup{1};
};

int command_run(const RunOptions& options);

}
