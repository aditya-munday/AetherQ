#include <string>

#include <gtest/gtest.h>

#include "aetherq/runtime/config.hpp"

using namespace aetherq;

TEST(Config, DefaultsAreValid) {
  EXPECT_TRUE(validate(default_config()).ok());
}

TEST(Config, ParsesValidJson) {
  const std::string text = R"({
    "threads": 4,
    "scheduler": "thread_pool",
    "allocator": "arena",
    "backend": "optimized",
    "parallel_for": "guided",
    "chunk_grain": 32,
    "seed": 12345,
    "measurement": {
      "warmup_iterations": 2,
      "repetitions": 10,
      "min_samples": 5,
      "correctness_mode": true
    }
  })";
  auto config = parse_config(text);
  ASSERT_TRUE(config.ok());
  EXPECT_EQ(config.value().threads, 4U);
  EXPECT_EQ(config.value().scheduler, SchedulerPolicy::thread_pool);
  EXPECT_EQ(config.value().allocator, AllocatorKind::arena);
  EXPECT_EQ(config.value().backend, Backend::optimized);
  EXPECT_EQ(config.value().parallel_for, ParallelForPolicy::guided);
  EXPECT_EQ(config.value().chunk_grain, 32U);
  EXPECT_EQ(config.value().seed, 12345U);
  EXPECT_EQ(config.value().measurement.repetitions, 10U);
  EXPECT_TRUE(config.value().measurement.correctness_mode);
}

TEST(Config, RejectsUnknownScheduler) {
  auto config = parse_config(R"({"threads": 2, "scheduler": "magic"})");
  ASSERT_FALSE(config.ok());
  EXPECT_EQ(config.error().code(), ErrorCode::invalid_config);
}

TEST(Config, RejectsMalformedJson) {
  auto config = parse_config("{not json");
  ASSERT_FALSE(config.ok());
  EXPECT_EQ(config.error().code(), ErrorCode::invalid_config);
}

TEST(Config, RejectsWrongType) {
  auto config = parse_config(R"({"threads": "many"})");
  ASSERT_FALSE(config.ok());
}

TEST(Config, RejectsZeroThreads) {
  auto config = parse_config(R"({"threads": 0})");
  ASSERT_FALSE(config.ok());
}

TEST(Config, RejectsSerialWithManyThreads) {
  auto config = parse_config(R"({"threads": 4, "scheduler": "serial"})");
  ASSERT_FALSE(config.ok());
}

TEST(Config, RejectsMinSamplesAboveRepetitions) {
  auto config = parse_config(R"({"measurement": {"repetitions": 2, "min_samples": 5}})");
  ASSERT_FALSE(config.ok());
}

TEST(Config, RejectsNegativeThreads) {
  auto config = parse_config(R"({"threads": -1})");
  ASSERT_FALSE(config.ok());
}

TEST(Config, RoundTripsThroughJson) {
  RuntimeConfig config = default_config();
  config.threads = 3;
  config.scheduler = SchedulerPolicy::thread_pool;
  config.allocator = AllocatorKind::pool;
  config.backend = Backend::optimized;
  config.seed = 777;
  auto parsed = parse_config(to_json(config));
  ASSERT_TRUE(parsed.ok());
  EXPECT_EQ(parsed.value().threads, 3U);
  EXPECT_EQ(parsed.value().allocator, AllocatorKind::pool);
  EXPECT_EQ(parsed.value().backend, Backend::optimized);
  EXPECT_EQ(parsed.value().seed, 777U);
}

TEST(Config, ParsesBackendNames) {
  EXPECT_TRUE(parse_backend_name("scalar").ok());
  EXPECT_TRUE(parse_backend_name("optimized").ok());
  EXPECT_FALSE(parse_backend_name("cuda").ok());
}

TEST(Config, ParsesAllocatorNames) {
  EXPECT_TRUE(parse_allocator_name("system").ok());
  EXPECT_TRUE(parse_allocator_name("arena").ok());
  EXPECT_TRUE(parse_allocator_name("pool").ok());
  EXPECT_FALSE(parse_allocator_name("gpu").ok());
}
