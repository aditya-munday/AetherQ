#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

#include "aetherq/runtime/result.hpp"

namespace aetherq {

struct ExecutorMetrics {
  std::uint64_t tasks_submitted{0};
  std::uint64_t tasks_completed{0};
  std::size_t queue_high_water_mark{0};
  std::uint64_t idle_waits{0};
  std::uint64_t contention_events{0};
};

using TaskFunction = std::function<void()>;

class CancellationToken {
public:
  CancellationToken() : flag_(std::make_shared<std::atomic<bool>>(false)) {}

  void cancel() noexcept {
    flag_->store(true, std::memory_order_relaxed);
  }
  [[nodiscard]] bool cancelled() const noexcept {
    return flag_->load(std::memory_order_relaxed);
  }

private:
  std::shared_ptr<std::atomic<bool>> flag_;
};

class Executor {
public:
  virtual ~Executor() = default;

  Executor(const Executor&) = delete;
  Executor& operator=(const Executor&) = delete;
  Executor(Executor&&) = delete;
  Executor& operator=(Executor&&) = delete;

  virtual Result<void> submit(TaskFunction task) = 0;
  virtual Result<void> wait() = 0;
  virtual void request_stop() = 0;
  [[nodiscard]] virtual std::size_t worker_count() const noexcept = 0;
  [[nodiscard]] virtual ExecutorMetrics metrics() const = 0;
  [[nodiscard]] virtual const char* name() const noexcept = 0;

protected:
  Executor() = default;
};

Result<std::unique_ptr<Executor>> make_executor(std::size_t workers, std::size_t queue_capacity);

}
