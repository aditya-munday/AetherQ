#pragma once

#include <string>

namespace aetherq::cli {

struct CompareOptions {
  std::string baseline;
  std::string candidate;
  bool json_output{false};
};

int command_compare(const CompareOptions& options);

}
