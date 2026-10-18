#include "command_run.hpp"

#include <iostream>

#include <nlohmann/json.hpp>

#include "aetherq/runtime/runtime.hpp"
#include "aetherq/runtime/version.hpp"
#include "aetherq/workloads/workload.hpp"

#include "cli_common.hpp"

namespace aetherq::cli {

int command_run(const RunOptions& options) {
  CommonOptions common;
  common.threads = options.threads;
  common.backend = options.backend;
  common.allocator = options.allocator;
  common.config_path = options.config_path;
  common.seed = options.seed;
  common.json_output = options.json_output;
  common.correctness_mode = options.correctness_mode;
  common.repetitions = options.repetitions;
  common.warmup = options.warmup;

  auto config = build_config(common);
  if (!config) {
    print_error(config.error());
    return 2;
  }

  auto workload = make_workload(options.workload);
  if (!workload) {
    print_error(workload.error());
    return 2;
  }

  Logger& logger = default_logger();
  auto runtime = Runtime::create(config.value(), logger);
  if (!runtime) {
    print_error(runtime.error());
    return 2;
  }

  WorkloadInputs inputs;
  inputs.paths = options.paths;
  inputs.options = options.options;
  inputs.assets = options.assets;
  inputs.observations = options.observations;
  inputs.items = options.items;

  RunReport report;
  auto executed = workload.value()->run(*runtime.value(), inputs, report);
  if (!executed) {
    print_error(executed.error());
    return 3;
  }

  auto validation = validate_report(report);
  if (!validation) {
    print_error(validation.error());
    return 4;
  }

  if (!options.output.empty()) {
    auto written = write_report_file(report, options.output);
    if (!written) {
      print_error(written.error());
      return 5;
    }
  }

  if (options.json_output) {
    std::cout << to_json(report) << '\n';
  } else {
    std::cout << to_text(report);
  }
  return report.correctness.passed ? 0 : 1;
}

}
