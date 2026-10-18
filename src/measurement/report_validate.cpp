#include "aetherq/measurement/report.hpp"

namespace aetherq {

Result<void> validate_report(const RunReport& report) {
  if (report.schema_version != report_schema_version) {
    return Error{ErrorCode::schema_violation,
                 "unsupported schema_version: " + std::to_string(report.schema_version)};
  }
  if (report.workload.empty()) {
    return Error{ErrorCode::schema_violation, "workload name must not be empty"};
  }
  if (report.backend.empty()) {
    return Error{ErrorCode::schema_violation, "backend must not be empty"};
  }
  if (report.timing.repetitions == 0) {
    return Error{ErrorCode::schema_violation, "timing.repetitions must be at least 1"};
  }
  if (report.timing.stats.sample_count != report.timing.samples_ns.size()) {
    return Error{ErrorCode::schema_violation,
                 "timing.stats.sample_count does not match samples_ns length"};
  }
  if (report.counters.available) {
    if (!report.counters.cycles || !report.counters.instructions) {
      return Error{ErrorCode::schema_violation,
                   "counters marked available must include cycles and instructions"};
    }
  }
  return ok_result();
}

}
