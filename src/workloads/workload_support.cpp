#include "workload_support.hpp"

#include <string>

#include "aetherq/runtime/version.hpp"

namespace aetherq {

RunReport make_report(Runtime& runtime, const std::string& workload, std::size_t threads) {
  RunReport report;
  report.schema_version = report_schema_version;
  report.run_id =
      workload + "-" + std::to_string(runtime.config().seed) + "-" + std::to_string(threads);
  report.workload = workload;
  report.backend = std::string(to_string(runtime.config().backend));
  report.seed = runtime.config().seed;
  report.environment = capture_environment();
  report.config = runtime.config();
  report.execution.scheduler = runtime.executor().name();
  report.execution.threads = threads;
  report.execution.parallel_efficiency = -1.0;
  report.memory.allocator = std::string(to_string(runtime.config().allocator));
  report.counters = probe_hardware_counters();
  return report;
}

void finalize_report(Runtime& runtime, RunReport& report) {
  const ExecutorMetrics metrics = runtime.executor().metrics();
  report.execution.tasks_submitted = metrics.tasks_submitted;
  report.execution.tasks_completed = metrics.tasks_completed;
  report.execution.queue_high_water_mark = metrics.queue_high_water_mark;
  report.memory.peak_rss_bytes = peak_rss_bytes();
}

}
