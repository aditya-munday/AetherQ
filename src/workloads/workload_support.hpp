#pragma once

#include <string>

#include "aetherq/executor/executor.hpp"
#include "aetherq/measurement/environment.hpp"
#include "aetherq/measurement/perf_counters.hpp"
#include "aetherq/measurement/report.hpp"
#include "aetherq/runtime/runtime.hpp"

namespace aetherq {

RunReport make_report(Runtime& runtime, const std::string& workload, std::size_t threads);
void finalize_report(Runtime& runtime, RunReport& report);

}
