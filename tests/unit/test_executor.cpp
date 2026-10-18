#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "aetherq/executor/executor.hpp"
#include "aetherq/executor/parallel_for.hpp"
#include "aetherq/executor/serial_executor.hpp"
#include "aetherq/executor/thread_pool_executor.hpp"
#include "aetherq/runtime/result.hpp"

using namespace aetherq;

TEST(SerialExecutor, ExecutesTasksInOrder) {
  SerialExecutor executor(16);
  std::vector<int> order;
  for (int i = 0; i < 5; ++i) {
    ASSERT_TRUE(executor.submit([&order, i] { order.push_back(i); }));
  }
  ASSERT_TRUE(executor.wait());
  ASSERT_EQ(order.size(), 5U);
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(order[static_cast<std::size_t>(i)], i);
  }
  EXPECT_EQ(executor.metrics().tasks_completed, 5U);
}

TEST(SerialExecutor, RejectsWhenQueueFull) {
  SerialExecutor executor(2);
  std::atomic<int> count{0};
  auto task = [&count] { count.fetch_add(1); };
  ASSERT_TRUE(executor.submit(task));
  ASSERT_TRUE(executor.submit(task));
  ASSERT_TRUE(executor.submit(task));
  EXPECT_EQ(count.load(), 3);
}

TEST(SerialExecutor, StopsAcceptingWork) {
  SerialExecutor executor(4);
  executor.request_stop();
  EXPECT_FALSE(executor.submit([] {}));
}

TEST(ThreadPoolExecutor, ExecutesAllTasksUnderBackpressure) {
  ThreadPoolExecutor executor(4, 256);
  std::atomic<int> counter{0};
  constexpr int total = 1000;
  int submitted = 0;
  while (submitted < total) {
    if (executor.submit([&counter] { counter.fetch_add(1); })) {
      ++submitted;
    } else {
      ASSERT_TRUE(executor.wait());
    }
  }
  ASSERT_TRUE(executor.wait());
  EXPECT_EQ(counter.load(), total);
  const ExecutorMetrics metrics = executor.metrics();
  EXPECT_EQ(metrics.tasks_completed, static_cast<std::uint64_t>(total));
}

TEST(ThreadPoolExecutor, AppliesBackpressure) {
  constexpr std::size_t workers = 2;
  constexpr std::size_t capacity = 8;
  ThreadPoolExecutor executor(workers, capacity);
  std::atomic<bool> release{false};
  std::size_t accepted = 0;
  bool rejected = false;
  for (std::size_t attempt = 0; attempt < workers + capacity + 4; ++attempt) {
    if (executor.submit([&release] {
          while (!release.load()) {
            std::this_thread::yield();
          }
        })) {
      ++accepted;
    } else {
      rejected = true;
      break;
    }
  }
  EXPECT_TRUE(rejected);
  EXPECT_GE(accepted, 1U);
  EXPECT_LE(accepted, workers + capacity);
  release.store(true);
  ASSERT_TRUE(executor.wait());
}

TEST(ThreadPoolExecutor, PropagatesTaskFailure) {
  ThreadPoolExecutor executor(2, 16);
  ASSERT_TRUE(executor.submit([] { throw std::runtime_error("boom"); }));
  auto result = executor.wait();
  EXPECT_FALSE(result.ok());
}

TEST(ThreadPoolExecutor, PropagatesErrorException) {
  ThreadPoolExecutor executor(2, 16);
  ASSERT_TRUE(executor.submit(
      [] { throw ErrorException(Error{ErrorCode::numerical_failure, "bad math"}); }));
  auto result = executor.wait();
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(result.error().code(), ErrorCode::numerical_failure);
}

TEST(ThreadPoolExecutor, ShutdownDuringLoad) {
  ThreadPoolExecutor executor(4, 512);
  std::atomic<int> counter{0};
  for (int i = 0; i < 200; ++i) {
    executor.submit([&counter] { counter.fetch_add(1); });
  }
  executor.request_stop();
  EXPECT_FALSE(executor.submit([] {}));
}

TEST(ParallelFor, CoversEveryIndexOnce) {
  ThreadPoolExecutor executor(4, 256);
  constexpr std::size_t count = 1000;
  std::vector<std::atomic<int>> hits(count);
  for (auto& hit : hits) {
    hit.store(0);
  }
  ASSERT_TRUE(parallel_for(executor, count, ParallelForPolicy::static_chunk, 64,
                           [&hits](std::size_t begin, std::size_t end) {
                             for (std::size_t i = begin; i < end; ++i) {
                               hits[i].fetch_add(1);
                             }
                           }));
  for (const auto& hit : hits) {
    EXPECT_EQ(hit.load(), 1);
  }
}

TEST(ParallelFor, GuidedPolicyCoversRange) {
  ThreadPoolExecutor executor(4, 256);
  constexpr std::size_t count = 777;
  std::atomic<std::size_t> sum{0};
  ASSERT_TRUE(parallel_for(executor, count, ParallelForPolicy::guided, 16,
                           [&sum](std::size_t begin, std::size_t end) {
                             for (std::size_t i = begin; i < end; ++i) {
                               sum.fetch_add(i);
                             }
                           }));
  EXPECT_EQ(sum.load(), (count - 1) * count / 2);
}

TEST(ParallelFor, FixedGrainCoversRange) {
  ThreadPoolExecutor executor(3, 256);
  constexpr std::size_t count = 100;
  std::atomic<std::size_t> total{0};
  ASSERT_TRUE(
      parallel_for(executor, count, ParallelForPolicy::fixed_grain, 7,
                   [&total](std::size_t begin, std::size_t end) { total.fetch_add(end - begin); }));
  EXPECT_EQ(total.load(), count);
}

TEST(ParallelFor, EmptyRangeIsNoop) {
  ThreadPoolExecutor executor(2, 16);
  EXPECT_TRUE(parallel_for(executor, 0, ParallelForPolicy::static_chunk, 8,
                           [](std::size_t, std::size_t) {}));
}

TEST(MakeExecutor, ValidatesArguments) {
  EXPECT_FALSE(make_executor(0, 10).ok());
  EXPECT_FALSE(make_executor(2, 0).ok());
  EXPECT_TRUE(make_executor(1, 10).ok());
  EXPECT_TRUE(make_executor(4, 10).ok());
}

TEST(CancellationToken, ReflectsState) {
  CancellationToken token;
  EXPECT_FALSE(token.cancelled());
  token.cancel();
  EXPECT_TRUE(token.cancelled());
}
