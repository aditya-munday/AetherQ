#pragma once

#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <vector>

#include "aetherq/executor/executor.hpp"

namespace aetherq {

class SerialExecutor final : public Executor {
public:
  explicit SerialExecutor(std::size_t queue_capacity);

  Result<void> submit(TaskFunction task) override;
  Result<void> wait() override;
  void request_stop() override;
  [[nodiscard]] std::size_t worker_count() const noexcept override;
  [[nodiscard]] ExecutorMetrics metrics() const override;
  [[nodiscard]] const char* name() const noexcept override;

private:
  mutable std::mutex mutex_;
  std::vector<TaskFunction> queue_;
  std::size_t capacity_;
  std::size_t high_water_mark_{0};
  std::uint64_t submitted_{0};
  std::uint64_t completed_{0};
  bool stopped_{false};
};

}
