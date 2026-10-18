#include <string>

#include <gtest/gtest.h>

#include "aetherq/measurement/report.hpp"

using namespace aetherq;

namespace {

RunReport make_sample_report() {
  RunReport report;
  report.run_id = "monte_carlo-1-1";
  report.workload = "monte_carlo";
  report.backend = "scalar";
  report.seed = 42;
  report.environment.cpu_model = "test-cpu";
  report.environment.logical_cores = 4;
  report.environment.compiler = "gcc";
  report.environment.build_type = "Release";
  report.timing.repetitions = 5;
  report.timing.warmup_iterations = 1;
  report.timing.samples_ns = {1000.0, 1100.0, 1200.0, 1300.0, 1400.0};
  report.timing.stats = summarize(report.timing.samples_ns);
  report.counters.available = false;
  report.counters.unavailable_reason = "perf_event_paranoid=4";
  return report;
}

}

TEST(Report, JsonRoundTripPreservesFields) {
  const RunReport report = make_sample_report();
  const std::string json = to_json(report);
  auto parsed = report_from_json(json);
  ASSERT_TRUE(parsed.ok());
  EXPECT_EQ(parsed.value().workload, report.workload);
  EXPECT_EQ(parsed.value().seed, report.seed);
  EXPECT_EQ(parsed.value().environment.cpu_model, report.environment.cpu_model);
  EXPECT_EQ(parsed.value().timing.repetitions, report.timing.repetitions);
  EXPECT_EQ(parsed.value().timing.samples_ns.size(), report.timing.samples_ns.size());
  EXPECT_DOUBLE_EQ(parsed.value().timing.stats.median, report.timing.stats.median);
}

TEST(Report, UnavailableCountersStayUnavailable) {
  const RunReport report = make_sample_report();
  auto parsed = report_from_json(to_json(report));
  ASSERT_TRUE(parsed.ok());
  EXPECT_FALSE(parsed.value().counters.available);
  EXPECT_FALSE(parsed.value().counters.cycles.has_value());
  EXPECT_EQ(parsed.value().counters.unavailable_reason, "perf_event_paranoid=4");
}

TEST(Report, ValidationRejectsWrongSchemaVersion) {
  RunReport report = make_sample_report();
  report.schema_version = 999;
  EXPECT_FALSE(validate_report(report).ok());
}

TEST(Report, ValidationRejectsEmptyWorkload) {
  RunReport report = make_sample_report();
  report.workload.clear();
  EXPECT_FALSE(validate_report(report).ok());
}

TEST(Report, ValidationRejectsSampleCountMismatch) {
  RunReport report = make_sample_report();
  report.timing.stats.sample_count = 99;
  EXPECT_FALSE(validate_report(report).ok());
}

TEST(Report, ValidationRejectsAvailableCountersWithoutValues) {
  RunReport report = make_sample_report();
  report.counters.available = true;
  EXPECT_FALSE(validate_report(report).ok());
}

TEST(Report, RejectsMalformedJson) {
  auto parsed = report_from_json("{bad");
  EXPECT_FALSE(parsed.ok());
}

TEST(Report, CsvContainsEnvironmentAndTiming) {
  const RunReport report = make_sample_report();
  const std::string csv = to_csv(report);
  EXPECT_NE(csv.find("environment,cpu_model,test-cpu"), std::string::npos);
  EXPECT_NE(csv.find("timing,median_ns"), std::string::npos);
  EXPECT_NE(csv.find("run,workload,monte_carlo"), std::string::npos);
}

TEST(Report, TextSummaryIsReadable) {
  const RunReport report = make_sample_report();
  const std::string text = to_text(report);
  EXPECT_NE(text.find("Workload: monte_carlo"), std::string::npos);
  EXPECT_NE(text.find("perf_event_paranoid"), std::string::npos);
}
