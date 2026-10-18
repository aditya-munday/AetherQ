#include <sstream>

#include "aetherq/measurement/report.hpp"

namespace aetherq {

std::string to_csv(const RunReport& report) {
  std::ostringstream out;
  out << "section,key,value\n";
  out << "run,run_id," << report.run_id << '\n';
  out << "run,workload," << report.workload << '\n';
  out << "run,backend," << report.backend << '\n';
  out << "run,seed," << report.seed << '\n';
  out << "environment,cpu_model," << report.environment.cpu_model << '\n';
  out << "environment,logical_cores," << report.environment.logical_cores << '\n';
  out << "environment,physical_cores," << report.environment.physical_cores << '\n';
  out << "environment,compiler," << report.environment.compiler << '\n';
  out << "environment,compiler_version," << report.environment.compiler_version << '\n';
  out << "environment,build_type," << report.environment.build_type << '\n';
  out << "environment,git_commit," << report.environment.git_commit << '\n';
  out << "environment,timestamp_utc," << report.environment.timestamp_utc << '\n';
  out << "execution,scheduler," << report.execution.scheduler << '\n';
  out << "execution,threads," << report.execution.threads << '\n';
  out << "execution,tasks_submitted," << report.execution.tasks_submitted << '\n';
  out << "execution,tasks_completed," << report.execution.tasks_completed << '\n';
  out << "execution,queue_high_water_mark," << report.execution.queue_high_water_mark << '\n';
  out << "execution,parallel_efficiency," << report.execution.parallel_efficiency << '\n';
  out << "memory,allocator," << report.memory.allocator << '\n';
  out << "memory,allocation_count," << report.memory.allocation_count << '\n';
  out << "memory,reuse_count," << report.memory.reuse_count << '\n';
  out << "memory,peak_live_bytes," << report.memory.peak_live_bytes << '\n';
  out << "memory,peak_rss_bytes," << report.memory.peak_rss_bytes << '\n';
  out << "timing,median_ns," << report.timing.stats.median << '\n';
  out << "timing,mean_ns," << report.timing.stats.mean << '\n';
  out << "timing,stddev_ns," << report.timing.stats.stddev << '\n';
  out << "timing,min_ns," << report.timing.stats.min << '\n';
  out << "timing,max_ns," << report.timing.stats.max << '\n';
  if (report.timing.stats.p50) {
    out << "timing,p50_ns," << *report.timing.stats.p50 << '\n';
  }
  if (report.timing.stats.p95) {
    out << "timing,p95_ns," << *report.timing.stats.p95 << '\n';
  }
  if (report.timing.stats.p99) {
    out << "timing,p99_ns," << *report.timing.stats.p99 << '\n';
  }
  out << "correctness,passed," << (report.correctness.passed ? 1 : 0) << '\n';
  out << "correctness,error," << report.correctness.error << '\n';
  out << "correctness,tolerance," << report.correctness.tolerance << '\n';
  for (const auto& metric : report.metrics) {
    out << "metric," << metric.name << ',' << metric.value << '\n';
  }
  return out.str();
}

}
