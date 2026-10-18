#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "aetherq/measurement/environment.hpp"
#include "aetherq/measurement/perf_counters.hpp"
#include "aetherq/measurement/sampler.hpp"
#include "aetherq/measurement/stats.hpp"
#include "aetherq/memory/allocation_metrics.hpp"
#include "aetherq/runtime/config.hpp"
#include "aetherq/runtime/result.hpp"

namespace aetherq {

constexpr int report_schema_version = 1;

struct CorrectnessOutcome {
  bool passed{false};
  std::string oracle;
  double measured{0.0};
  double expected{0.0};
  double tolerance{0.0};
  double error{0.0};
  std::string detail;
};

struct MetricEntry {
  std::string name;
  double value{0.0};
  std::string unit;
};

struct MemoryReport {
  std::uint64_t allocation_count{0};
  std::uint64_t deallocation_count{0};
  std::uint64_t reuse_count{0};
  std::size_t requested_bytes{0};
  std::size_t peak_live_bytes{0};
  std::size_t peak_rss_bytes{0};
  std::string allocator;
};

struct ExecutionReport {
  std::string scheduler;
  std::size_t threads{0};
  std::uint64_t tasks_submitted{0};
  std::uint64_t tasks_completed{0};
  std::size_t queue_high_water_mark{0};
  double parallel_efficiency{0.0};
};

struct RunReport {
  int schema_version{report_schema_version};
  std::string run_id;
  std::string workload;
  std::string backend;
  std::uint64_t seed{0};
  EnvironmentInfo environment;
  RuntimeConfig config;
  ExecutionReport execution;
  MemoryReport memory;
  HardwareCounters counters;
  SampleResult timing;
  std::map<std::string, double> workload_parameters;
  std::vector<MetricEntry> metrics;
  CorrectnessOutcome correctness;
};

std::string to_json(const RunReport& report);
Result<RunReport> report_from_json(std::string_view json_text);
Result<void> validate_report(const RunReport& report);
Result<void> write_report_file(const RunReport& report, const std::string& path);
Result<RunReport> read_report_file(const std::string& path);
std::string to_csv(const RunReport& report);
std::string to_text(const RunReport& report);

}
