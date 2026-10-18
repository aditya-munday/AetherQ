#pragma once

#include <string>

namespace aetherq::cli {

struct ValidateOptions {
  std::string input;
  bool json_output{false};
};

int command_validate(const ValidateOptions& options);

}
