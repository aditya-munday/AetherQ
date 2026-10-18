#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>

#include "aetherq/runtime/version.hpp"
#include "aetherq/workloads/workload.hpp"

#include "command_bench.hpp"
#include "command_compare.hpp"
#include "command_report.hpp"
#include "command_run.hpp"
#include "command_system_info.hpp"
#include "command_validate.hpp"

namespace {

std::vector<std::size_t> parse_thread_list(const std::string& text) {
  std::vector<std::size_t> counts;
  std::stringstream stream(text);
  std::string token;
  while (std::getline(stream, token, ',')) {
    if (!token.empty()) {
      counts.push_back(static_cast<std::size_t>(std::stoul(token)));
    }
  }
  return counts;
}

}

int main(int argc, char** argv) {
  CLI::App app{"AetherQ reproducible quantitative runtime"};
  app.set_version_flag("--version", std::string(aetherq::version()));
  app.require_subcommand(1);

  auto* system_info = app.add_subcommand("system-info", "Report hardware and toolchain details");
  bool system_info_json = false;
  system_info->add_flag("--json", system_info_json, "Machine-readable JSON output");

  auto* run = app.add_subcommand("run", "Execute a workload");
  aetherq::cli::RunOptions run_options;
  run->add_option("workload", run_options.workload, "Workload name")->required();
  run->add_option("--paths", run_options.paths, "Monte Carlo path count");
  run->add_option("--options", run_options.options, "Option batch size");
  run->add_option("--assets", run_options.assets, "Asset count");
  run->add_option("--observations", run_options.observations, "Observation count");
  run->add_option("--items", run_options.items, "Item count");
  run->add_option("--seed", run_options.seed, "Random seed");
  run->add_option("--backend", run_options.backend, "scalar|optimized");
  run->add_option("--allocator", run_options.allocator, "system|arena|pool");
  run->add_option("--threads", run_options.threads, "Worker thread count");
  run->add_option("--repetitions", run_options.repetitions, "Measured repetitions");
  run->add_option("--warmup", run_options.warmup, "Warmup iterations");
  run->add_option("--config", run_options.config_path, "Versioned JSON config file");
  run->add_option("--output", run_options.output, "Report output path");
  run->add_flag("--json", run_options.json_output, "Machine-readable JSON output");
  run->add_flag("--correctness", run_options.correctness_mode, "Correctness-only run");

  auto* bench = app.add_subcommand("bench", "Benchmark suites");
  bench->require_subcommand(1);

  auto* bench_allocation = bench->add_subcommand("allocation", "Compare allocators");
  aetherq::cli::BenchAllocationOptions allocation_options;
  bench_allocation->add_option("--items", allocation_options.items, "Item count");
  bench_allocation->add_option("--allocator", allocation_options.allocator,
                               "system|arena|pool (default: all three)");
  bench_allocation->add_option("--repetitions", allocation_options.repetitions, "Repetitions");
  bench_allocation->add_option("--warmup", allocation_options.warmup, "Warmup iterations");
  bench_allocation->add_option("--seed", allocation_options.seed, "Random seed");
  bench_allocation->add_flag("--json", allocation_options.json_output, "JSON output");

  auto* bench_scaling = bench->add_subcommand("scaling", "Thread scaling sweep");
  aetherq::cli::BenchScalingOptions scaling_options;
  bench_scaling->add_option("--workload", scaling_options.workload, "Workload name");
  std::string thread_list = "1,2,4";
  bench_scaling->add_option("--threads", thread_list, "Comma separated thread counts");
  bench_scaling->add_option("--paths", scaling_options.paths, "Monte Carlo path count");
  bench_scaling->add_option("--options", scaling_options.options, "Option batch size");
  bench_scaling->add_option("--assets", scaling_options.assets, "Asset count");
  bench_scaling->add_option("--observations", scaling_options.observations, "Observation count");
  bench_scaling->add_option("--backend", scaling_options.backend, "scalar|optimized");
  bench_scaling->add_option("--repetitions", scaling_options.repetitions, "Repetitions");
  bench_scaling->add_option("--warmup", scaling_options.warmup, "Warmup iterations");
  bench_scaling->add_option("--seed", scaling_options.seed, "Random seed");
  bench_scaling->add_flag("--json", scaling_options.json_output, "JSON output");

  auto* bench_kernels = bench->add_subcommand("kernels", "Kernel microbenchmarks");
  aetherq::cli::BenchKernelsOptions kernel_options;
  bench_kernels->add_option("--filter", kernel_options.filter, "Kernel name filter");
  bench_kernels->add_flag("--json", kernel_options.json_output, "JSON output");

  auto* compare = app.add_subcommand("compare", "Noise-aware run comparison");
  aetherq::cli::CompareOptions compare_options;
  compare->add_option("baseline", compare_options.baseline, "Baseline report")->required();
  compare->add_option("candidate", compare_options.candidate, "Candidate report")->required();
  compare->add_flag("--json", compare_options.json_output, "JSON output");

  auto* report = app.add_subcommand("report", "Render a report");
  aetherq::cli::ReportOptions report_options;
  report->add_option("input", report_options.input, "Report path")->required();
  report->add_option("--format", report_options.format, "text|csv|markdown|json");

  auto* validate = app.add_subcommand("validate", "Validate a report against the schema");
  aetherq::cli::ValidateOptions validate_options;
  validate->add_option("input", validate_options.input, "Report path")->required();
  validate->add_flag("--json", validate_options.json_output, "JSON output");

  CLI11_PARSE(app, argc, argv);

  if (*system_info) {
    return aetherq::cli::command_system_info(system_info_json);
  }
  if (*run) {
    return aetherq::cli::command_run(run_options);
  }
  if (*bench_allocation) {
    return aetherq::cli::command_bench_allocation(allocation_options);
  }
  if (*bench_scaling) {
    scaling_options.thread_counts = parse_thread_list(thread_list);
    return aetherq::cli::command_bench_scaling(scaling_options);
  }
  if (*bench_kernels) {
    return aetherq::cli::command_bench_kernels(kernel_options);
  }
  if (*compare) {
    return aetherq::cli::command_compare(compare_options);
  }
  if (*report) {
    return aetherq::cli::command_report(report_options);
  }
  if (*validate) {
    return aetherq::cli::command_validate(validate_options);
  }
  std::cerr << "no subcommand selected\n";
  return 2;
}
