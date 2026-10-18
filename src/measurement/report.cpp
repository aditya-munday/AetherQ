#include "aetherq/measurement/report.hpp"

#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "aetherq/runtime/config.hpp"

namespace aetherq {

namespace {

nlohmann::json stats_to_json(const SummaryStats& stats) {
  nlohmann::json document;
  document["sample_count"] = stats.sample_count;
  document["mean"] = stats.mean;
  document["stddev"] = stats.stddev;
  document["median"] = stats.median;
  document["mad"] = stats.mad;
  document["min"] = stats.min;
  document["max"] = stats.max;
  document["p50"] = stats.p50 ? nlohmann::json(*stats.p50) : nlohmann::json(nullptr);
  document["p95"] = stats.p95 ? nlohmann::json(*stats.p95) : nlohmann::json(nullptr);
  document["p99"] = stats.p99 ? nlohmann::json(*stats.p99) : nlohmann::json(nullptr);
  document["p999"] = stats.p999 ? nlohmann::json(*stats.p999) : nlohmann::json(nullptr);
  return document;
}

SummaryStats stats_from_json(const nlohmann::json& document) {
  SummaryStats stats;
  stats.sample_count = document.value("sample_count", std::size_t{0});
  stats.mean = document.value("mean", 0.0);
  stats.stddev = document.value("stddev", 0.0);
  stats.median = document.value("median", 0.0);
  stats.mad = document.value("mad", 0.0);
  stats.min = document.value("min", 0.0);
  stats.max = document.value("max", 0.0);
  const auto read_optional = [&document](const char* key) -> std::optional<double> {
    if (!document.contains(key) || document[key].is_null()) {
      return std::nullopt;
    }
    return document[key].get<double>();
  };
  stats.p50 = read_optional("p50");
  stats.p95 = read_optional("p95");
  stats.p99 = read_optional("p99");
  stats.p999 = read_optional("p999");
  return stats;
}

}

std::string to_json(const RunReport& report) {
  nlohmann::json document;
  document["schema_version"] = report.schema_version;
  document["run_id"] = report.run_id;
  document["workload"] = report.workload;
  document["backend"] = report.backend;
  document["seed"] = report.seed;

  auto& environment = document["environment"];
  environment["cpu_model"] = report.environment.cpu_model;
  environment["logical_cores"] = report.environment.logical_cores;
  environment["physical_cores"] = report.environment.physical_cores;
  environment["l1_cache_bytes"] = report.environment.l1_cache_bytes;
  environment["l2_cache_bytes"] = report.environment.l2_cache_bytes;
  environment["l3_cache_bytes"] = report.environment.l3_cache_bytes;
  environment["frequency_governor"] = report.environment.frequency_governor;
  environment["os_name"] = report.environment.os_name;
  environment["kernel_version"] = report.environment.kernel_version;
  environment["compiler"] = report.environment.compiler;
  environment["compiler_version"] = report.environment.compiler_version;
  environment["build_type"] = report.environment.build_type;
  environment["git_commit"] = report.environment.git_commit;
  environment["git_dirty"] = report.environment.git_dirty;
  environment["timestamp_utc"] = report.environment.timestamp_utc;

  auto& config = document["config"];
  config["threads"] = report.config.threads;
  config["scheduler"] = to_string(report.config.scheduler);
  config["allocator"] = to_string(report.config.allocator);
  config["backend"] = to_string(report.config.backend);
  config["parallel_for"] = to_string(report.config.parallel_for);
  config["chunk_grain"] = report.config.chunk_grain;
  config["seed"] = report.config.seed;
  config["measurement"]["warmup_iterations"] = report.config.measurement.warmup_iterations;
  config["measurement"]["repetitions"] = report.config.measurement.repetitions;
  config["measurement"]["min_samples"] = report.config.measurement.min_samples;
  config["measurement"]["correctness_mode"] = report.config.measurement.correctness_mode;

  auto& execution = document["execution"];
  execution["scheduler"] = report.execution.scheduler;
  execution["threads"] = report.execution.threads;
  execution["tasks_submitted"] = report.execution.tasks_submitted;
  execution["tasks_completed"] = report.execution.tasks_completed;
  execution["queue_high_water_mark"] = report.execution.queue_high_water_mark;
  execution["parallel_efficiency"] = report.execution.parallel_efficiency;

  auto& memory = document["memory"];
  memory["allocator"] = report.memory.allocator;
  memory["allocation_count"] = report.memory.allocation_count;
  memory["deallocation_count"] = report.memory.deallocation_count;
  memory["reuse_count"] = report.memory.reuse_count;
  memory["requested_bytes"] = report.memory.requested_bytes;
  memory["peak_live_bytes"] = report.memory.peak_live_bytes;
  memory["peak_rss_bytes"] = report.memory.peak_rss_bytes;

  auto& counters = document["counters"];
  counters["available"] = report.counters.available;
  counters["unavailable_reason"] = report.counters.unavailable_reason;
  counters["cycles"] =
      report.counters.cycles ? nlohmann::json(*report.counters.cycles) : nlohmann::json(nullptr);
  counters["instructions"] = report.counters.instructions
                                 ? nlohmann::json(*report.counters.instructions)
                                 : nlohmann::json(nullptr);
  counters["cache_misses"] = report.counters.cache_misses
                                 ? nlohmann::json(*report.counters.cache_misses)
                                 : nlohmann::json(nullptr);
  counters["branch_misses"] = report.counters.branch_misses
                                  ? nlohmann::json(*report.counters.branch_misses)
                                  : nlohmann::json(nullptr);
  counters["ipc"] =
      report.counters.ipc ? nlohmann::json(*report.counters.ipc) : nlohmann::json(nullptr);

  auto& timing = document["timing"];
  timing["warmup_iterations"] = report.timing.warmup_iterations;
  timing["repetitions"] = report.timing.repetitions;
  timing["samples_ns"] = report.timing.samples_ns;
  timing["stats"] = stats_to_json(report.timing.stats);

  document["workload_parameters"] = report.workload_parameters;

  auto& metrics = document["metrics"];
  metrics = nlohmann::json::array();
  for (const auto& entry : report.metrics) {
    nlohmann::json item;
    item["name"] = entry.name;
    item["value"] = entry.value;
    item["unit"] = entry.unit;
    metrics.push_back(item);
  }

  auto& correctness = document["correctness"];
  correctness["passed"] = report.correctness.passed;
  correctness["oracle"] = report.correctness.oracle;
  correctness["measured"] = report.correctness.measured;
  correctness["expected"] = report.correctness.expected;
  correctness["tolerance"] = report.correctness.tolerance;
  correctness["error"] = report.correctness.error;
  correctness["detail"] = report.correctness.detail;

  return document.dump(2);
}

Result<RunReport> report_from_json(std::string_view json_text) {
  nlohmann::json document;
  try {
    document = nlohmann::json::parse(json_text);
  } catch (const nlohmann::json::parse_error& error) {
    return Error{ErrorCode::schema_violation, std::string("json parse error: ") + error.what()};
  }
  if (!document.is_object()) {
    return Error{ErrorCode::schema_violation, "report root must be an object"};
  }
  RunReport report;
  report.schema_version = document.value("schema_version", 0);
  report.run_id = document.value("run_id", std::string{});
  report.workload = document.value("workload", std::string{});
  report.backend = document.value("backend", std::string{});
  report.seed = document.value("seed", std::uint64_t{0});

  if (document.contains("environment")) {
    const auto& environment = document["environment"];
    report.environment.cpu_model = environment.value("cpu_model", std::string{});
    report.environment.logical_cores = environment.value("logical_cores", std::size_t{0});
    report.environment.physical_cores = environment.value("physical_cores", std::size_t{0});
    report.environment.l1_cache_bytes = environment.value("l1_cache_bytes", std::size_t{0});
    report.environment.l2_cache_bytes = environment.value("l2_cache_bytes", std::size_t{0});
    report.environment.l3_cache_bytes = environment.value("l3_cache_bytes", std::size_t{0});
    report.environment.frequency_governor = environment.value("frequency_governor", std::string{});
    report.environment.os_name = environment.value("os_name", std::string{});
    report.environment.kernel_version = environment.value("kernel_version", std::string{});
    report.environment.compiler = environment.value("compiler", std::string{});
    report.environment.compiler_version = environment.value("compiler_version", std::string{});
    report.environment.build_type = environment.value("build_type", std::string{});
    report.environment.git_commit = environment.value("git_commit", std::string{});
    report.environment.git_dirty = environment.value("git_dirty", std::string{});
    report.environment.timestamp_utc = environment.value("timestamp_utc", std::string{});
  }

  if (document.contains("config")) {
    const auto& config = document["config"];
    report.config.threads = config.value("threads", std::size_t{1});
    report.config.chunk_grain = config.value("chunk_grain", std::size_t{1});
    report.config.seed = config.value("seed", std::uint64_t{0});
    if (config.contains("scheduler") && config["scheduler"].is_string()) {
      const std::string value = config["scheduler"].get<std::string>();
      report.config.scheduler =
          value == "thread_pool" ? SchedulerPolicy::thread_pool : SchedulerPolicy::serial;
    }
    if (config.contains("allocator") && config["allocator"].is_string()) {
      const std::string value = config["allocator"].get<std::string>();
      if (value == "arena") {
        report.config.allocator = AllocatorKind::arena;
      } else if (value == "pool") {
        report.config.allocator = AllocatorKind::pool;
      } else {
        report.config.allocator = AllocatorKind::system;
      }
    }
    if (config.contains("backend") && config["backend"].is_string()) {
      report.config.backend = config["backend"].get<std::string>() == "optimized"
                                  ? Backend::optimized
                                  : Backend::scalar;
    }
    if (config.contains("measurement")) {
      const auto& measurement = config["measurement"];
      report.config.measurement.warmup_iterations =
          measurement.value("warmup_iterations", std::size_t{0});
      report.config.measurement.repetitions = measurement.value("repetitions", std::size_t{1});
      report.config.measurement.min_samples = measurement.value("min_samples", std::size_t{1});
      report.config.measurement.correctness_mode = measurement.value("correctness_mode", false);
    }
  }

  if (document.contains("execution")) {
    const auto& execution = document["execution"];
    report.execution.scheduler = execution.value("scheduler", std::string{});
    report.execution.threads = execution.value("threads", std::size_t{0});
    report.execution.tasks_submitted = execution.value("tasks_submitted", std::uint64_t{0});
    report.execution.tasks_completed = execution.value("tasks_completed", std::uint64_t{0});
    report.execution.queue_high_water_mark =
        execution.value("queue_high_water_mark", std::size_t{0});
    report.execution.parallel_efficiency = execution.value("parallel_efficiency", 0.0);
  }

  if (document.contains("memory")) {
    const auto& memory = document["memory"];
    report.memory.allocator = memory.value("allocator", std::string{});
    report.memory.allocation_count = memory.value("allocation_count", std::uint64_t{0});
    report.memory.deallocation_count = memory.value("deallocation_count", std::uint64_t{0});
    report.memory.reuse_count = memory.value("reuse_count", std::uint64_t{0});
    report.memory.requested_bytes = memory.value("requested_bytes", std::size_t{0});
    report.memory.peak_live_bytes = memory.value("peak_live_bytes", std::size_t{0});
    report.memory.peak_rss_bytes = memory.value("peak_rss_bytes", std::size_t{0});
  }

  if (document.contains("counters")) {
    const auto& counters = document["counters"];
    report.counters.available = counters.value("available", false);
    report.counters.unavailable_reason = counters.value("unavailable_reason", std::string{});
    const auto read_optional = [&counters](const char* key) -> std::optional<std::uint64_t> {
      if (!counters.contains(key) || counters[key].is_null()) {
        return std::nullopt;
      }
      return counters[key].get<std::uint64_t>();
    };
    report.counters.cycles = read_optional("cycles");
    report.counters.instructions = read_optional("instructions");
    report.counters.cache_misses = read_optional("cache_misses");
    report.counters.branch_misses = read_optional("branch_misses");
    if (counters.contains("ipc") && !counters["ipc"].is_null()) {
      report.counters.ipc = counters["ipc"].get<double>();
    }
  }

  if (document.contains("timing")) {
    const auto& timing = document["timing"];
    report.timing.warmup_iterations = timing.value("warmup_iterations", std::size_t{0});
    report.timing.repetitions = timing.value("repetitions", std::size_t{0});
    if (timing.contains("samples_ns")) {
      report.timing.samples_ns = timing["samples_ns"].get<std::vector<double>>();
    }
    if (timing.contains("stats")) {
      report.timing.stats = stats_from_json(timing["stats"]);
    }
  }

  if (document.contains("workload_parameters")) {
    report.workload_parameters =
        document["workload_parameters"].get<std::map<std::string, double>>();
  }

  if (document.contains("metrics")) {
    for (const auto& item : document["metrics"]) {
      MetricEntry entry;
      entry.name = item.value("name", std::string{});
      entry.value = item.value("value", 0.0);
      entry.unit = item.value("unit", std::string{});
      report.metrics.push_back(entry);
    }
  }

  if (document.contains("correctness")) {
    const auto& correctness = document["correctness"];
    report.correctness.passed = correctness.value("passed", false);
    report.correctness.oracle = correctness.value("oracle", std::string{});
    report.correctness.measured = correctness.value("measured", 0.0);
    report.correctness.expected = correctness.value("expected", 0.0);
    report.correctness.tolerance = correctness.value("tolerance", 0.0);
    report.correctness.error = correctness.value("error", 0.0);
    report.correctness.detail = correctness.value("detail", std::string{});
  }

  auto validation = validate_report(report);
  if (!validation) {
    return validation.error();
  }
  return report;
}

Result<void> write_report_file(const RunReport& report, const std::string& path) {
  std::ofstream stream(path);
  if (!stream) {
    return Error{ErrorCode::io_error, "cannot open output file: " + path};
  }
  stream << to_json(report) << '\n';
  if (!stream) {
    return Error{ErrorCode::io_error, "failed writing output file: " + path};
  }
  return ok_result();
}

Result<RunReport> read_report_file(const std::string& path) {
  std::ifstream stream(path);
  if (!stream) {
    return Error{ErrorCode::io_error, "cannot open report file: " + path};
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return report_from_json(buffer.str());
}

std::string to_text(const RunReport& report) {
  std::ostringstream out;
  out << "Workload: " << report.workload << " | Backend: " << report.backend << "/"
      << report.execution.scheduler << '\n';
  out << "Correctness: " << (report.correctness.passed ? "PASS" : "FAIL") << " ("
      << report.correctness.detail << ") | measured=" << report.correctness.measured
      << " expected=" << report.correctness.expected
      << " tolerance=" << report.correctness.tolerance << '\n';
  out << "Threads: " << report.execution.threads
      << " | median ms: " << report.timing.stats.median / 1e6 << " | peak RSS MiB: "
      << static_cast<double>(report.memory.peak_rss_bytes) / (1024.0 * 1024.0) << '\n';
  out << "Seed: " << report.seed << " | Build: " << report.environment.build_type
      << " | Commit: " << report.environment.git_commit << '\n';
  out << "Counters: "
      << (report.counters.available ? std::string("available") : report.counters.unavailable_reason)
      << '\n';
  for (const auto& metric : report.metrics) {
    out << "  " << metric.name << ": " << metric.value << ' ' << metric.unit << '\n';
  }
  return out.str();
}

}
