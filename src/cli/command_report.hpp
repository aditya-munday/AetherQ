#pragma once

#include <string>

namespace aetherq::cli {

struct ReportOptions {
  std::string input;
  std::string format{"text"};
};

int command_report(const ReportOptions& options);

}
