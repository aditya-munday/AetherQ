#pragma once

#include <memory>

#include "aetherq/executor/executor.hpp"
#include "aetherq/runtime/config.hpp"
#include "aetherq/runtime/logging.hpp"
#include "aetherq/runtime/result.hpp"

namespace aetherq {

class Runtime {
public:
  static Result<std::unique_ptr<Runtime>> create(const RuntimeConfig& config, Logger& logger);

  Runtime(Runtime&&) noexcept;
  Runtime& operator=(Runtime&&) noexcept;
  Runtime(const Runtime&) = delete;
  Runtime& operator=(const Runtime&) = delete;
  ~Runtime();

  [[nodiscard]] const RuntimeConfig& config() const noexcept;
  [[nodiscard]] Executor& executor() noexcept;
  [[nodiscard]] Logger& logger() noexcept;

private:
  Runtime(RuntimeConfig config, std::unique_ptr<Executor> executor, Logger& logger);

  RuntimeConfig config_;
  std::unique_ptr<Executor> executor_;
  Logger* logger_;
};

}
