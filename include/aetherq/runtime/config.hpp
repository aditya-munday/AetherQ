#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "aetherq/runtime/result.hpp"

namespace aetherq {

enum class SchedulerPolicy : std::uint8_t { serial, thread_pool };
enum class AllocatorKind : std::uint8_t { system, arena, pool };
enum class Backend : std::uint8_t { scalar, optimized };
enum class ParallelForPolicy : std::uint8_t { static_chunk, guided, fixed_grain };

std::string_view to_string(SchedulerPolicy value) noexcept;
std::string_view to_string(AllocatorKind value) noexcept;
std::string_view to_string(Backend value) noexcept;
std::string_view to_string(ParallelForPolicy value) noexcept;

constexpr int config_schema_version = 1;

struct MeasurementConfig {
  std::size_t warmup_iterations{1};
  std::size_t repetitions{5};
  std::size_t min_samples{5};
  bool correctness_mode{false};
};

struct RuntimeConfig {
  std::size_t threads{1};
  SchedulerPolicy scheduler{SchedulerPolicy::serial};
  AllocatorKind allocator{AllocatorKind::system};
  Backend backend{Backend::scalar};
  ParallelForPolicy parallel_for{ParallelForPolicy::static_chunk};
  std::size_t chunk_grain{64};
  std::uint64_t seed{20260101};
  MeasurementConfig measurement{};
};

RuntimeConfig default_config();

Result<RuntimeConfig> parse_config(std::string_view json_text);
Result<void> validate(const RuntimeConfig& config);
std::string to_json(const RuntimeConfig& config);

Result<RuntimeConfig> load_config_file(const std::string& path);

Result<Backend> parse_backend_name(const std::string& name);
Result<AllocatorKind> parse_allocator_name(const std::string& name);

}
