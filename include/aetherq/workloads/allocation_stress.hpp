#pragma once

#include <cstddef>
#include <memory>

#include "aetherq/memory/allocation_metrics.hpp"
#include "aetherq/runtime/result.hpp"
#include "aetherq/workloads/workload.hpp"

namespace aetherq {

struct AllocationStressResult {
  std::size_t items{0};
  double checksum{0.0};
  std::size_t allocations{0};
};

AllocationStressResult allocation_stress(Runtime& runtime, std::size_t items, AllocatorKind kind,
                                         AllocationRecorder* recorder = nullptr);

class AllocationStressWorkload final : public Workload {
public:
  [[nodiscard]] std::string_view name() const noexcept override;
  Result<void> run(Runtime& runtime, const WorkloadInputs& inputs, RunReport& report) override;
};

}
