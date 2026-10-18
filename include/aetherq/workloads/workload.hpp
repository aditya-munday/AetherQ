#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "aetherq/measurement/report.hpp"
#include "aetherq/runtime/config.hpp"
#include "aetherq/runtime/result.hpp"

namespace aetherq {

class Runtime;

struct WorkloadInputs {
  std::size_t paths{100000};
  std::size_t options{100000};
  std::size_t assets{32};
  std::size_t observations{1024};
  std::size_t items{100000};
};

class Workload {
public:
  virtual ~Workload() = default;

  Workload(const Workload&) = delete;
  Workload& operator=(const Workload&) = delete;
  Workload(Workload&&) = delete;
  Workload& operator=(Workload&&) = delete;

  [[nodiscard]] virtual std::string_view name() const noexcept = 0;
  virtual Result<void> run(Runtime& runtime, const WorkloadInputs& inputs, RunReport& report) = 0;

protected:
  Workload() = default;
};

Result<std::unique_ptr<Workload>> make_workload(std::string_view name);
std::string_view available_workloads() noexcept;

}
