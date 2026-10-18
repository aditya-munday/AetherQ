#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include "aetherq/runtime/result.hpp"
#include "aetherq/workloads/workload.hpp"

namespace aetherq {

struct FactorRiskResult {
  double portfolio_variance{0.0};
  double portfolio_volatility{0.0};
  double marginal_risk_sum{0.0};
  double component_risk_sum{0.0};
};

FactorRiskResult factor_risk_scalar(std::span<const double> returns, std::size_t observations,
                                    std::size_t assets, std::span<const double> weights);

Result<double> factor_risk_reference(std::span<const double> returns, std::size_t observations,
                                     std::size_t assets, std::span<const double> weights);

class FactorRiskWorkload final : public Workload {
public:
  [[nodiscard]] std::string_view name() const noexcept override;
  Result<void> run(Runtime& runtime, const WorkloadInputs& inputs, RunReport& report) override;
};

}
