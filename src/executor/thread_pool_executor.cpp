#include "aetherq/executor/thread_pool_executor.hpp"

#include <string>
#include <utility>

namespace aetherq {

ThreadPoolExecutor::ThreadPoolExecutor(std::size_t workers, std::size_t queue_capacity)
    : capacity_(queue_capacity) {
  if (capacity_ == 0) {
    capacity_ = 1;
  }
  workers_.reserve(workers);
  for (std::size_t i = 0; i < workers; ++i) {
    workers_.emplace_back([this] { worker_loop(); });
  }
}

ThreadPoolExecutor::~ThreadPoolExecutor() {
  request_stop();
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

Result<void> ThreadPoolExecutor::submit(TaskFunction task) {
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (stopping_) {
      return Error{ErrorCode::concurrency_error, "executor is stopping"};
    }
    if (queue_.size() >= capacity_) {
      return Error{ErrorCode::concurrency_error, "executor queue is full"};
    }
    queue_.push_back(std::move(task));
    submitted_ += 1;
    if (queue_.size() > high_water_mark_) {
      high_water_mark_ = queue_.size();
    }
  }
  work_available_.notify_one();
  return ok_result();
}

Result<void> ThreadPoolExecutor::wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  work_drained_.wait(lock, [this] { return queue_.empty() && active_ == 0; });
  if (first_failure_ != nullptr) {
    const std::exception_ptr failure = first_failure_;
    first_failure_ = nullptr;
    const std::size_t count = failure_count_;
    failure_count_ = 0;
    lock.unlock();
    try {
      std::rethrow_exception(failure);
    } catch (const ErrorException& error) {
      return error.error();
    } catch (const std::exception& error) {
      return Error{ErrorCode::concurrency_error,
                   std::string("task failure (") + std::to_string(count) + "): " + error.what()};
    } catch (...) {
      return Error{ErrorCode::concurrency_error, "task failure: unknown exception"};
    }
  }
  return ok_result();
}

void ThreadPoolExecutor::request_stop() {
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    stopping_ = true;
    queue_.clear();
  }
  work_available_.notify_all();
  work_drained_.notify_all();
}

std::size_t ThreadPoolExecutor::worker_count() const noexcept {
  return workers_.size();
}

ExecutorMetrics ThreadPoolExecutor::metrics() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  ExecutorMetrics metrics;
  metrics.tasks_submitted = submitted_;
  metrics.tasks_completed = completed_;
  metrics.queue_high_water_mark = high_water_mark_;
  metrics.idle_waits = idle_waits_;
  metrics.contention_events = 0;
  return metrics;
}

const char* ThreadPoolExecutor::name() const noexcept {
  return "ThreadPoolExecutor";
}

void ThreadPoolExecutor::record_failure(std::exception_ptr failure) {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (first_failure_ == nullptr) {
    first_failure_ = failure;
  }
  failure_count_ += 1;
}

void ThreadPoolExecutor::worker_loop() {
  for (;;) {
    TaskFunction task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (queue_.empty() && stopping_) {
        work_drained_.notify_all();
        return;
      }
      if (queue_.empty()) {
        idle_waits_ += 1;
        work_available_.wait(lock, [this] { return !queue_.empty() || stopping_; });
        if (queue_.empty() && stopping_) {
          work_drained_.notify_all();
          return;
        }
      }
      task = std::move(queue_.front());
      queue_.pop_front();
      active_ += 1;
    }
    try {
      task();
    } catch (...) {
      record_failure(std::current_exception());
    }
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      active_ -= 1;
      completed_ += 1;
      if (queue_.empty() && active_ == 0) {
        work_drained_.notify_all();
      }
    }
  }
}

}
