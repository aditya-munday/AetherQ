#include "aetherq/measurement/compare.hpp"

#include <cmath>
#include <sstream>

#include "aetherq/measurement/stats.hpp"

namespace aetherq {

namespace {

const char* verdict_name(Verdict verdict) {
  switch (verdict) {
    case Verdict::improved:
      return "improved";
    case Verdict::regressed:
      return "regressed";
    case Verdict::within_noise:
      return "within_noise";
  }
  return "unknown";
}

MetricComparison compare_metric(const std::string& name, double baseline, double candidate,
                                const SummaryStats& baseline_stats,
                                const SummaryStats& candidate_stats) {
  MetricComparison comparison;
  comparison.name = name;
  comparison.baseline = baseline;
  comparison.candidate = candidate;
  comparison.relative_delta = baseline != 0.0 ? (candidate - baseline) / baseline : 0.0;
  if (distributions_overlap(baseline_stats, candidate_stats)) {
    comparison.verdict = Verdict::within_noise;
  } else if (candidate < baseline) {
    comparison.verdict = Verdict::improved;
  } else {
    comparison.verdict = Verdict::regressed;
  }
  return comparison;
}

}

ComparisonResult compare_reports(const RunReport& baseline, const RunReport& candidate) {
  ComparisonResult result;
  const auto& lhs = baseline.environment;
  const auto& rhs = candidate.environment;
  if (lhs.cpu_model != rhs.cpu_model) {
    result.environment_mismatch = true;
    result.environment_warnings.push_back("cpu_model differs: " + lhs.cpu_model + " vs " +
                                          rhs.cpu_model);
  }
  if (lhs.logical_cores != rhs.logical_cores) {
    result.environment_mismatch = true;
    result.environment_warnings.push_back("logical_cores differ");
  }
  if (lhs.compiler != rhs.compiler || lhs.compiler_version != rhs.compiler_version) {
    result.environment_mismatch = true;
    result.environment_warnings.push_back("compiler differs: " + lhs.compiler + " vs " +
                                          rhs.compiler);
  }
  if (lhs.build_type != rhs.build_type) {
    result.environment_mismatch = true;
    result.environment_warnings.push_back("build_type differs: " + lhs.build_type + " vs " +
                                          rhs.build_type);
  }

  result.metrics.push_back(compare_metric("median_ns", baseline.timing.stats.median,
                                          candidate.timing.stats.median, baseline.timing.stats,
                                          candidate.timing.stats));
  result.metrics.push_back(compare_metric("mean_ns", baseline.timing.stats.mean,
                                          candidate.timing.stats.mean, baseline.timing.stats,
                                          candidate.timing.stats));
  result.metrics.push_back(compare_metric("stddev_ns", baseline.timing.stats.stddev,
                                          candidate.timing.stats.stddev, baseline.timing.stats,
                                          candidate.timing.stats));
  result.metrics.push_back(compare_metric("peak_rss_bytes",
                                          static_cast<double>(baseline.memory.peak_rss_bytes),
                                          static_cast<double>(candidate.memory.peak_rss_bytes),
                                          baseline.timing.stats, candidate.timing.stats));
  result.metrics.push_back(compare_metric("allocation_count",
                                          static_cast<double>(baseline.memory.allocation_count),
                                          static_cast<double>(candidate.memory.allocation_count),
                                          baseline.timing.stats, candidate.timing.stats));
  return result;
}

std::string format_comparison(const ComparisonResult& comparison) {
  std::ostringstream out;
  if (comparison.environment_mismatch) {
    out << "WARNING: environments differ; comparison is not a controlled experiment\n";
    for (const auto& warning : comparison.environment_warnings) {
      out << "  - " << warning << '\n';
    }
  }
  out << "metric,baseline,candidate,relative_delta,verdict\n";
  for (const auto& metric : comparison.metrics) {
    out << metric.name << ',' << metric.baseline << ',' << metric.candidate << ','
        << metric.relative_delta << ',' << verdict_name(metric.verdict) << '\n';
  }
  return out.str();
}

}
