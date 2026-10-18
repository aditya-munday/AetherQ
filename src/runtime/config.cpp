#include "aetherq/runtime/config.hpp"

#include <fstream>
#include <limits>
#include <sstream>

#include <nlohmann/json.hpp>

namespace aetherq {

std::string_view to_string(SchedulerPolicy value) noexcept {
  switch (value) {
    case SchedulerPolicy::serial:
      return "serial";
    case SchedulerPolicy::thread_pool:
      return "thread_pool";
  }
  return "unknown";
}

std::string_view to_string(AllocatorKind value) noexcept {
  switch (value) {
    case AllocatorKind::system:
      return "system";
    case AllocatorKind::arena:
      return "arena";
    case AllocatorKind::pool:
      return "pool";
  }
  return "unknown";
}

std::string_view to_string(Backend value) noexcept {
  switch (value) {
    case Backend::scalar:
      return "scalar";
    case Backend::optimized:
      return "optimized";
  }
  return "unknown";
}

std::string_view to_string(ParallelForPolicy value) noexcept {
  switch (value) {
    case ParallelForPolicy::static_chunk:
      return "static_chunk";
    case ParallelForPolicy::guided:
      return "guided";
    case ParallelForPolicy::fixed_grain:
      return "fixed_grain";
  }
  return "unknown";
}

RuntimeConfig default_config() {
  return RuntimeConfig{};
}

namespace {

Result<SchedulerPolicy> parse_scheduler(const std::string& value) {
  if (value == "serial") {
    return SchedulerPolicy::serial;
  }
  if (value == "thread_pool") {
    return SchedulerPolicy::thread_pool;
  }
  return Error{ErrorCode::invalid_config, "unknown scheduler: " + value};
}

Result<AllocatorKind> parse_allocator(const std::string& value) {
  if (value == "system") {
    return AllocatorKind::system;
  }
  if (value == "arena") {
    return AllocatorKind::arena;
  }
  if (value == "pool") {
    return AllocatorKind::pool;
  }
  return Error{ErrorCode::invalid_config, "unknown allocator: " + value};
}

Result<Backend> parse_backend(const std::string& value) {
  if (value == "scalar") {
    return Backend::scalar;
  }
  if (value == "optimized") {
    return Backend::optimized;
  }
  return Error{ErrorCode::invalid_config, "unknown backend: " + value};
}

Result<ParallelForPolicy> parse_parallel_for(const std::string& value) {
  if (value == "static_chunk") {
    return ParallelForPolicy::static_chunk;
  }
  if (value == "guided") {
    return ParallelForPolicy::guided;
  }
  if (value == "fixed_grain") {
    return ParallelForPolicy::fixed_grain;
  }
  return Error{ErrorCode::invalid_config, "unknown parallel_for policy: " + value};
}

}

Result<RuntimeConfig> parse_config(std::string_view json_text) {
  nlohmann::json document;
  try {
    document = nlohmann::json::parse(json_text);
  } catch (const nlohmann::json::parse_error& error) {
    return Error{ErrorCode::invalid_config, std::string("json parse error: ") + error.what()};
  }
  if (!document.is_object()) {
    return Error{ErrorCode::invalid_config, "config root must be an object"};
  }

  if (document.contains("schema_version")) {
    if (!document["schema_version"].is_number_integer()) {
      return Error{ErrorCode::invalid_config, "schema_version must be an integer"};
    }
    const auto version = document["schema_version"].get<long long>();
    if (version != config_schema_version) {
      return Error{ErrorCode::invalid_config, "unsupported config schema_version " +
                                                  std::to_string(version) + " (expected " +
                                                  std::to_string(config_schema_version) + ")"};
    }
  }

  RuntimeConfig config = default_config();

  if (document.contains("threads")) {
    if (!document["threads"].is_number_unsigned() && !document["threads"].is_number_integer()) {
      return Error{ErrorCode::invalid_config, "threads must be an integer"};
    }
    const auto value = document["threads"].get<long long>();
    if (value < 0) {
      return Error{ErrorCode::invalid_config, "threads must be non-negative"};
    }
    config.threads = static_cast<std::size_t>(value);
  }

  if (document.contains("scheduler")) {
    if (!document["scheduler"].is_string()) {
      return Error{ErrorCode::invalid_config, "scheduler must be a string"};
    }
    auto parsed = parse_scheduler(document["scheduler"].get<std::string>());
    if (!parsed) {
      return parsed.error();
    }
    config.scheduler = parsed.value();
  }

  if (document.contains("allocator")) {
    if (!document["allocator"].is_string()) {
      return Error{ErrorCode::invalid_config, "allocator must be a string"};
    }
    auto parsed = parse_allocator(document["allocator"].get<std::string>());
    if (!parsed) {
      return parsed.error();
    }
    config.allocator = parsed.value();
  }

  if (document.contains("backend")) {
    if (!document["backend"].is_string()) {
      return Error{ErrorCode::invalid_config, "backend must be a string"};
    }
    auto parsed = parse_backend(document["backend"].get<std::string>());
    if (!parsed) {
      return parsed.error();
    }
    config.backend = parsed.value();
  }

  if (document.contains("parallel_for")) {
    if (!document["parallel_for"].is_string()) {
      return Error{ErrorCode::invalid_config, "parallel_for must be a string"};
    }
    auto parsed = parse_parallel_for(document["parallel_for"].get<std::string>());
    if (!parsed) {
      return parsed.error();
    }
    config.parallel_for = parsed.value();
  }

  if (document.contains("chunk_grain")) {
    if (!document["chunk_grain"].is_number_unsigned()) {
      return Error{ErrorCode::invalid_config, "chunk_grain must be a non-negative integer"};
    }
    config.chunk_grain = document["chunk_grain"].get<std::size_t>();
  }

  if (document.contains("seed")) {
    if (!document["seed"].is_number_unsigned()) {
      return Error{ErrorCode::invalid_config, "seed must be a non-negative integer"};
    }
    config.seed = document["seed"].get<std::uint64_t>();
  }

  if (document.contains("measurement")) {
    const auto& measurement = document["measurement"];
    if (!measurement.is_object()) {
      return Error{ErrorCode::invalid_config, "measurement must be an object"};
    }
    if (measurement.contains("warmup_iterations")) {
      if (!measurement["warmup_iterations"].is_number_unsigned()) {
        return Error{ErrorCode::invalid_config, "warmup_iterations must be a non-negative integer"};
      }
      config.measurement.warmup_iterations = measurement["warmup_iterations"].get<std::size_t>();
    }
    if (measurement.contains("repetitions")) {
      if (!measurement["repetitions"].is_number_unsigned()) {
        return Error{ErrorCode::invalid_config, "repetitions must be a non-negative integer"};
      }
      config.measurement.repetitions = measurement["repetitions"].get<std::size_t>();
    }
    if (measurement.contains("min_samples")) {
      if (!measurement["min_samples"].is_number_unsigned()) {
        return Error{ErrorCode::invalid_config, "min_samples must be a non-negative integer"};
      }
      config.measurement.min_samples = measurement["min_samples"].get<std::size_t>();
    }
    if (measurement.contains("correctness_mode")) {
      if (!measurement["correctness_mode"].is_boolean()) {
        return Error{ErrorCode::invalid_config, "correctness_mode must be a boolean"};
      }
      config.measurement.correctness_mode = measurement["correctness_mode"].get<bool>();
    }
  }

  auto validation = validate(config);
  if (!validation) {
    return validation.error();
  }
  return config;
}

Result<void> validate(const RuntimeConfig& config) {
  if (config.threads == 0) {
    return Error{ErrorCode::invalid_config, "threads must be at least 1"};
  }
  if (config.threads > 1024) {
    return Error{ErrorCode::invalid_config, "threads exceeds supported maximum of 1024"};
  }
  if (config.chunk_grain == 0) {
    return Error{ErrorCode::invalid_config, "chunk_grain must be at least 1"};
  }
  if (config.measurement.repetitions == 0) {
    return Error{ErrorCode::invalid_config, "repetitions must be at least 1"};
  }
  if (config.measurement.min_samples == 0) {
    return Error{ErrorCode::invalid_config, "min_samples must be at least 1"};
  }
  if (config.measurement.min_samples > config.measurement.repetitions) {
    return Error{ErrorCode::invalid_config, "min_samples must not exceed repetitions"};
  }
  if (config.scheduler == SchedulerPolicy::serial && config.threads != 1) {
    return Error{ErrorCode::invalid_config, "serial scheduler requires threads == 1"};
  }
  return ok_result();
}

std::string to_json(const RuntimeConfig& config) {
  nlohmann::json document;
  document["threads"] = config.threads;
  document["scheduler"] = to_string(config.scheduler);
  document["allocator"] = to_string(config.allocator);
  document["backend"] = to_string(config.backend);
  document["parallel_for"] = to_string(config.parallel_for);
  document["chunk_grain"] = config.chunk_grain;
  document["seed"] = config.seed;
  document["measurement"]["warmup_iterations"] = config.measurement.warmup_iterations;
  document["measurement"]["repetitions"] = config.measurement.repetitions;
  document["measurement"]["min_samples"] = config.measurement.min_samples;
  document["measurement"]["correctness_mode"] = config.measurement.correctness_mode;
  return document.dump(2);
}

Result<Backend> parse_backend_name(const std::string& name) {
  return parse_backend(name);
}

Result<AllocatorKind> parse_allocator_name(const std::string& name) {
  return parse_allocator(name);
}

Result<RuntimeConfig> load_config_file(const std::string& path) {
  std::ifstream stream(path);
  if (!stream) {
    return Error{ErrorCode::io_error, "cannot open config file: " + path};
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return parse_config(buffer.str());
}

}
