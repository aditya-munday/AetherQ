#pragma once

#include <string>
#include <vector>

#include "aetherq/measurement/report.hpp"

namespace aetherq {

enum class Verdict { improved, regressed, within_noise };

struct MetricComparison {
  std::string name;
  double baseline;
  double candidate;
  double relative_delta;
  Verdict verdict;
};

struct ComparisonResult {
  bool environment_mismatch{false};
  std::vector<std::string> environment_warnings;
  std::vector<MetricComparison> metrics;
};

ComparisonResult compare_reports(const RunReport& baseline, const RunReport& candidate);
std::string format_comparison(const ComparisonResult& comparison);

}
