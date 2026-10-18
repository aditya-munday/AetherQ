#include "command_compare.hpp"

#include <iostream>

#include <nlohmann/json.hpp>

#include "aetherq/measurement/compare.hpp"
#include "aetherq/measurement/report.hpp"

#include "cli_common.hpp"

namespace aetherq::cli {

int command_compare(const CompareOptions& options) {
  auto baseline = read_report_file(options.baseline);
  if (!baseline) {
    print_error(baseline.error());
    return 2;
  }
  auto candidate = read_report_file(options.candidate);
  if (!candidate) {
    print_error(candidate.error());
    return 2;
  }
  const ComparisonResult comparison = compare_reports(baseline.value(), candidate.value());
  if (options.json_output) {
    nlohmann::json document;
    document["environment_mismatch"] = comparison.environment_mismatch;
    document["environment_warnings"] = comparison.environment_warnings;
    nlohmann::json metrics = nlohmann::json::array();
    for (const auto& metric : comparison.metrics) {
      nlohmann::json item;
      item["name"] = metric.name;
      item["baseline"] = metric.baseline;
      item["candidate"] = metric.candidate;
      item["relative_delta"] = metric.relative_delta;
      switch (metric.verdict) {
        case Verdict::improved:
          item["verdict"] = "improved";
          break;
        case Verdict::regressed:
          item["verdict"] = "regressed";
          break;
        case Verdict::within_noise:
          item["verdict"] = "within_noise";
          break;
      }
      metrics.push_back(item);
    }
    document["metrics"] = metrics;
    std::cout << document.dump(2) << '\n';
    return 0;
  }
  std::cout << format_comparison(comparison);
  return comparison.environment_mismatch ? 1 : 0;
}

}
