#include "aetherq/executor/executor.hpp"

#include <memory>
#include <string>

#include "aetherq/executor/serial_executor.hpp"
#include "aetherq/executor/thread_pool_executor.hpp"

namespace aetherq {

Result<std::unique_ptr<Executor>> make_executor(std::size_t workers, std::size_t queue_capacity) {
  if (workers == 0) {
    return Error{ErrorCode::invalid_argument, "executor requires at least one worker"};
  }
  if (queue_capacity == 0) {
    return Error{ErrorCode::invalid_argument, "executor requires a non-zero queue capacity"};
  }
  if (workers == 1) {
    return std::unique_ptr<Executor>(new SerialExecutor(queue_capacity));
  }
  return std::unique_ptr<Executor>(new ThreadPoolExecutor(workers, queue_capacity));
}

}
