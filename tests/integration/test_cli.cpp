#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace {

std::string shell_quote(const std::string& value) {
  return "'" + value + "'";
}

struct CommandResult {
  int exit_code{0};
  std::string output;
};

CommandResult run_command(const std::string& command) {
  CommandResult result;
  const std::string full = command + " 2>&1";
  std::array<char, 4096> buffer{};
  FILE* pipe = popen(full.c_str(), "r");
  if (pipe == nullptr) {
    result.exit_code = -1;
    return result;
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
    result.output += buffer.data();
  }
  const int status = pclose(pipe);
  result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
  return result;
}

std::string read_file(const std::string& path) {
  std::ifstream stream(path);
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

std::string binary_path() {
  return std::string(AETHERQ_CLI_PATH);
}

}

TEST(CliIntegration, SystemInfoSucceeds) {
  const CommandResult result = run_command(shell_quote(binary_path()) + " system-info");
  EXPECT_EQ(result.exit_code, 0);
  EXPECT_NE(result.output.find("AetherQ"), std::string::npos);
}

TEST(CliIntegration, SystemInfoJsonIsValid) {
  const CommandResult result = run_command(shell_quote(binary_path()) + " system-info --json");
  EXPECT_EQ(result.exit_code, 0);
  EXPECT_NE(result.output.find("\"logical_cores\""), std::string::npos);
  EXPECT_NE(result.output.find("\"cpu_features\""), std::string::npos);
}

TEST(CliIntegration, RunProducesValidReport) {
  const std::string output = "/tmp/aetherq_integration_run.json";
  std::remove(output.c_str());
  const CommandResult result =
      run_command(shell_quote(binary_path()) +
                  " run monte_carlo --paths 20000 --repetitions 2 --warmup 1 "
                  "--output " +
                  shell_quote(output) + " --json");
  EXPECT_EQ(result.exit_code, 0);
  const std::string content = read_file(output);
  EXPECT_NE(content.find("\"schema_version\""), std::string::npos);
  EXPECT_NE(content.find("\"environment\""), std::string::npos);

  const CommandResult validated =
      run_command(shell_quote(binary_path()) + " validate " + shell_quote(output));
  EXPECT_EQ(validated.exit_code, 0);

  const CommandResult reported =
      run_command(shell_quote(binary_path()) + " report " + shell_quote(output) + " --format csv");
  EXPECT_EQ(reported.exit_code, 0);
  EXPECT_NE(reported.output.find("timing,median_ns"), std::string::npos);

  const CommandResult compared = run_command(shell_quote(binary_path()) + " compare " +
                                             shell_quote(output) + " " + shell_quote(output));
  EXPECT_EQ(compared.exit_code, 0);
}

TEST(CliIntegration, InvalidWorkloadFails) {
  const CommandResult result =
      run_command(shell_quote(binary_path()) + " run not_a_workload --paths 100");
  EXPECT_NE(result.exit_code, 0);
}

TEST(CliIntegration, InvalidConfigFails) {
  const std::string config = "/tmp/aetherq_integration_bad.json";
  {
    std::ofstream stream(config);
    stream << "{\"threads\": 0}";
  }
  const CommandResult result =
      run_command(shell_quote(binary_path()) + " run monte_carlo --config " + shell_quote(config));
  EXPECT_NE(result.exit_code, 0);
  std::remove(config.c_str());
}

TEST(CliIntegration, ValidateRejectsMissingFile) {
  const CommandResult result =
      run_command(shell_quote(binary_path()) + " validate /tmp/aetherq_does_not_exist.json");
  EXPECT_NE(result.exit_code, 0);
}

TEST(CliIntegration, BenchAllocationRuns) {
  const CommandResult result =
      run_command(shell_quote(binary_path()) + " bench allocation --items 5000 --allocator arena");
  EXPECT_EQ(result.exit_code, 0);
}

TEST(CliIntegration, BenchKernelsRuns) {
  const CommandResult result = run_command(shell_quote(binary_path()) + " bench kernels --json");
  EXPECT_EQ(result.exit_code, 0);
  EXPECT_NE(result.output.find("\"rows\""), std::string::npos);
}
