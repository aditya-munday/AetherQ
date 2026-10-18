#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "aetherq/executor/executor.hpp"

namespace aetherq {

class ThreadPoolExecutor final : public Executor {
public:
  ThreadPoolExecutor(std::size_t workers, std::size_t queue_capacity);
  ~ThreadPoolExecutor() override;

  ThreadPoolExecutor(const ThreadPoolExecutor&) = delete;
  ThreadPoolExecutor& operator=(const ThreadPoolExecutor&) = delete;
  ThreadPoolExecutor(ThreadPoolExecutor&&) = delete;
  ThreadPoolExecutor& operator=(ThreadPoolExecutor&&) = delete;

  Result<void> submit(TaskFunction task) override;
  Result<void> wait() override;
  void request_stop() override;
  [[nodiscard]] std::size_t worker_count() const noexcept override;
  [[nodiscard]] ExecutorMetrics metrics() const override;
  [[nodiscard]] const char* name() const noexcept override;

private:
  void worker_loop();
  void record_failure(std::exception_ptr failure);

  mutable std::mutex mutex_;
  std::condition_variable work_available_;
  std::condition_variable work_drained_;
  std::deque<TaskFunction> queue_;
  std::vector<std::thread> workers_;
  std::size_t capacity_;
  std::size_t active_{0};
  std::size_t high_water_mark_{0};
  std::uint64_t submitted_{0};
  std::uint64_t completed_{0};
  std::uint64_t idle_waits_{0};
  bool stopping_{false};
  std::exception_ptr first_failure_;
  std::size_t failure_count_{0};
};

}
