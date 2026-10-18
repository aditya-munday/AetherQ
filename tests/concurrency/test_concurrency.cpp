#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "aetherq/executor/executor.hpp"
#include "aetherq/executor/parallel_for.hpp"
#include "aetherq/executor/thread_pool_executor.hpp"

using namespace aetherq;

TEST(Concurrency, ManyTasksManyRepetitions) {
  ThreadPoolExecutor executor(4, 4096);
  for (int repetition = 0; repetition < 20; ++repetition) {
    std::atomic<int> counter{0};
    constexpr int total = 500;
    for (int i = 0; i < total; ++i) {
      ASSERT_TRUE(executor.submit([&counter] { counter.fetch_add(1); }));
    }
    ASSERT_TRUE(executor.wait());
    EXPECT_EQ(counter.load(), total);
  }
}

TEST(Concurrency, ShutdownDuringLoadIsSafe) {
  for (int repetition = 0; repetition < 10; ++repetition) {
    ThreadPoolExecutor executor(4, 256);
    std::atomic<int> counter{0};
    for (int i = 0; i < 500; ++i) {
      executor.submit([&counter] {
        for (int spin = 0; spin < 100; ++spin) {
          static_cast<void>(spin);
        }
        counter.fetch_add(1);
      });
    }
    executor.request_stop();
  }
}

TEST(Concurrency, ExceptionPropagationUnderLoad) {
  ThreadPoolExecutor executor(4, 1024);
  for (int i = 0; i < 200; ++i) {
    executor.submit([] {});
  }
  ASSERT_TRUE(executor.submit([] { throw std::runtime_error("failure"); }));
  for (int i = 0; i < 200; ++i) {
    executor.submit([] {});
  }
  auto result = executor.wait();
  EXPECT_FALSE(result.ok());
}

TEST(Concurrency, RepeatedConstructionDestruction) {
  for (int repetition = 0; repetition < 50; ++repetition) {
    ThreadPoolExecutor executor(2, 64);
    std::atomic<int> counter{0};
    for (int i = 0; i < 32; ++i) {
      executor.submit([&counter] { counter.fetch_add(1); });
    }
    ASSERT_TRUE(executor.wait());
    EXPECT_EQ(counter.load(), 32);
  }
}

TEST(Concurrency, ParallelForNoLostOrDuplicateIndices) {
  ThreadPoolExecutor executor(4, 2048);
  constexpr std::size_t count = 5000;
  for (int repetition = 0; repetition < 10; ++repetition) {
    std::vector<std::atomic<int>> hits(count);
    for (auto& hit : hits) {
      hit.store(0);
    }
    ASSERT_TRUE(parallel_for(executor, count, ParallelForPolicy::guided, 32,
                             [&hits](std::size_t begin, std::size_t end) {
                               for (std::size_t i = begin; i < end; ++i) {
                                 hits[i].fetch_add(1);
                               }
                             }));
    std::size_t total = 0;
    for (const auto& hit : hits) {
      EXPECT_EQ(hit.load(), 1);
      total += static_cast<std::size_t>(hit.load());
    }
    EXPECT_EQ(total, count);
  }
}

TEST(Concurrency, QueueBackpressureUnderContention) {
  ThreadPoolExecutor executor(4, 32);
  std::atomic<bool> release{false};
  std::atomic<int> rejected{0};
  std::atomic<int> accepted{0};
  std::vector<std::thread> submitters;
  for (int thread_index = 0; thread_index < 4; ++thread_index) {
    submitters.emplace_back([&] {
      for (int i = 0; i < 200; ++i) {
        auto submitted = executor.submit([&release] {
          while (!release.load()) {
            std::this_thread::yield();
          }
        });
        if (submitted) {
          accepted.fetch_add(1);
        } else {
          rejected.fetch_add(1);
        }
      }
    });
  }
  for (auto& thread : submitters) {
    thread.join();
  }
  release.store(true);
  ASSERT_TRUE(executor.wait());
  EXPECT_GT(accepted.load(), 0);
  EXPECT_EQ(accepted.load() + rejected.load(), 800);
}
