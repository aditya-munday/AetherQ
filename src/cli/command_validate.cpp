#include "command_validate.hpp"

#include <iostream>

#include <nlohmann/json.hpp>

#include "aetherq/measurement/report.hpp"

#include "cli_common.hpp"

namespace aetherq::cli {

int command_validate(const ValidateOptions& options) {
  auto report = read_report_file(options.input);
  if (!report) {
    if (options.json_output) {
      nlohmann::json document;
      document["valid"] = false;
      document["error"] = report.error().to_string();
      std::cout << document.dump(2) << '\n';
    } else {
      print_error(report.error());
    }
    return 4;
  }
  auto validation = validate_report(report.value());
  if (!validation) {
    if (options.json_output) {
      nlohmann::json document;
      document["valid"] = false;
      document["error"] = validation.error().to_string();
      std::cout << document.dump(2) << '\n';
    } else {
      print_error(validation.error());
    }
    return 4;
  }
  if (options.json_output) {
    nlohmann::json document;
    document["valid"] = true;
    document["schema_version"] = report.value().schema_version;
    document["workload"] = report.value().workload;
    std::cout << document.dump(2) << '\n';
  } else {
    std::cout << "valid: schema_version=" << report.value().schema_version
              << " workload=" << report.value().workload << '\n';
  }
  return 0;
}

}
