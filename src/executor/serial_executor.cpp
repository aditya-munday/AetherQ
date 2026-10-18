#include "aetherq/executor/serial_executor.hpp"

#include <string>
#include <utility>
#include <vector>

namespace aetherq {

SerialExecutor::SerialExecutor(std::size_t queue_capacity) : capacity_(queue_capacity) {
  if (capacity_ == 0) {
    capacity_ = 1;
  }
  queue_.reserve(capacity_);
}

Result<void> SerialExecutor::submit(TaskFunction task) {
  std::vector<TaskFunction> ready;
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (stopped_) {
      return Error{ErrorCode::concurrency_error, "executor is stopped"};
    }
    if (queue_.size() >= capacity_) {
      return Error{ErrorCode::concurrency_error, "executor queue is full"};
    }
    queue_.push_back(std::move(task));
    submitted_ += 1;
    if (queue_.size() > high_water_mark_) {
      high_water_mark_ = queue_.size();
    }
    ready.swap(queue_);
  }
  for (auto& work : ready) {
    work();
    const std::lock_guard<std::mutex> lock(mutex_);
    completed_ += 1;
  }
  return ok_result();
}

Result<void> SerialExecutor::wait() {
  return ok_result();
}

void SerialExecutor::request_stop() {
  const std::lock_guard<std::mutex> lock(mutex_);
  stopped_ = true;
}

std::size_t SerialExecutor::worker_count() const noexcept {
  return 1;
}

ExecutorMetrics SerialExecutor::metrics() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  ExecutorMetrics metrics;
  metrics.tasks_submitted = submitted_;
  metrics.tasks_completed = completed_;
  metrics.queue_high_water_mark = high_water_mark_;
  metrics.idle_waits = 0;
  metrics.contention_events = 0;
  return metrics;
}

const char* SerialExecutor::name() const noexcept {
  return "SerialExecutor";
}

}
