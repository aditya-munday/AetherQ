#include "aetherq/runtime/runtime.hpp"

#include <utility>

#include "aetherq/executor/serial_executor.hpp"
#include "aetherq/executor/thread_pool_executor.hpp"

namespace aetherq {

namespace {
constexpr std::size_t default_queue_capacity = 4096;
}

Runtime::Runtime(RuntimeConfig config, std::unique_ptr<Executor> executor, Logger& logger)
    : config_(config), executor_(std::move(executor)), logger_(&logger) {}

Runtime::Runtime(Runtime&&) noexcept = default;
Runtime& Runtime::operator=(Runtime&&) noexcept = default;
Runtime::~Runtime() = default;

Result<std::unique_ptr<Runtime>> Runtime::create(const RuntimeConfig& config, Logger& logger) {
  auto validation = validate(config);
  if (!validation) {
    return validation.error();
  }
  std::unique_ptr<Executor> executor;
  if (config.scheduler == SchedulerPolicy::serial) {
    executor = std::make_unique<SerialExecutor>(default_queue_capacity);
  } else {
    executor = std::make_unique<ThreadPoolExecutor>(config.threads, default_queue_capacity);
  }
  logger.log(LogLevel::debug, "runtime constructed");
  return std::unique_ptr<Runtime>(new Runtime(config, std::move(executor), logger));
}

const RuntimeConfig& Runtime::config() const noexcept {
  return config_;
}
Executor& Runtime::executor() noexcept {
  return *executor_;
}
Logger& Runtime::logger() noexcept {
  return *logger_;
}

}
