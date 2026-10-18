#include "command_report.hpp"

#include <iostream>

#include "aetherq/measurement/report.hpp"

#include "cli_common.hpp"

namespace aetherq::cli {

int command_report(const ReportOptions& options) {
  auto report = read_report_file(options.input);
  if (!report) {
    print_error(report.error());
    return 2;
  }
  if (options.format == "json") {
    std::cout << to_json(report.value()) << '\n';
  } else if (options.format == "csv") {
    std::cout << to_csv(report.value());
  } else if (options.format == "markdown") {
    std::cout << "| metric | value |\n| --- | --- |\n";
    std::cout << "| workload | " << report.value().workload << " |\n";
    std::cout << "| backend | " << report.value().backend << " |\n";
    std::cout << "| threads | " << report.value().execution.threads << " |\n";
    std::cout << "| median_ms | " << report.value().timing.stats.median * 1e-6 << " |\n";
    std::cout << "| peak_rss_bytes | " << report.value().memory.peak_rss_bytes << " |\n";
    std::cout << "| correctness | " << (report.value().correctness.passed ? "PASS" : "FAIL")
              << " |\n";
  } else if (options.format == "text") {
    std::cout << to_text(report.value());
  } else {
    print_error(Error{ErrorCode::invalid_argument, "unknown format: " + options.format});
    return 2;
  }
  return 0;
}

}
