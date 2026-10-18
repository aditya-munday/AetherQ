#include "cli_common.hpp"

#include <iostream>

#include "aetherq/runtime/config.hpp"

namespace aetherq::cli {

Result<RuntimeConfig> build_config(const CommonOptions& options) {
  RuntimeConfig config = default_config();
  if (!options.config_path.empty()) {
    auto loaded = load_config_file(options.config_path);
    if (!loaded) {
      return loaded.error();
    }
    config = loaded.value();
  }

  config.threads = options.threads;
  config.seed = options.seed;
  config.measurement.repetitions = options.repetitions;
  config.measurement.warmup_iterations = options.warmup;
  config.measurement.min_samples = options.repetitions < 2 ? 1 : 2;
  config.measurement.correctness_mode = options.correctness_mode;

  auto backend = parse_backend_name(options.backend);
  if (!backend) {
    return backend.error();
  }
  config.backend = backend.value();

  auto allocator = parse_allocator_name(options.allocator);
  if (!allocator) {
    return allocator.error();
  }
  config.allocator = allocator.value();

  config.scheduler = config.threads > 1 ? SchedulerPolicy::thread_pool : SchedulerPolicy::serial;

  auto validation = validate(config);
  if (!validation) {
    return validation.error();
  }
  return config;
}

std::string escape_csv(const std::string& value) {
  if (value.find_first_of(",\"\n") == std::string::npos) {
    return value;
  }
  std::string escaped = "\"";
  for (const char character : value) {
    if (character == '"') {
      escaped += "\"\"";
    } else {
      escaped += character;
    }
  }
  escaped += "\"";
  return escaped;
}

void print_error(const Error& error) {
  std::cerr << "error: " << error.to_string() << '\n';
}

}
