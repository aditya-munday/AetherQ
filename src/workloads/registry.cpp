#include <memory>
#include <string>
#include <string_view>

#include "aetherq/workloads/allocation_stress.hpp"
#include "aetherq/workloads/black_scholes.hpp"
#include "aetherq/workloads/factor_risk.hpp"
#include "aetherq/workloads/monte_carlo.hpp"
#include "aetherq/workloads/workload.hpp"

namespace aetherq {

Result<std::unique_ptr<Workload>> make_workload(std::string_view name) {
  if (name == "monte_carlo") {
    return std::unique_ptr<Workload>(new MonteCarloWorkload());
  }
  if (name == "black_scholes") {
    return std::unique_ptr<Workload>(new BlackScholesWorkload());
  }
  if (name == "factor_risk") {
    return std::unique_ptr<Workload>(new FactorRiskWorkload());
  }
  if (name == "allocation_stress") {
    return std::unique_ptr<Workload>(new AllocationStressWorkload());
  }
  return Error{ErrorCode::invalid_argument, "unknown workload: " + std::string(name)};
}

std::string_view available_workloads() noexcept {
  return "monte_carlo, black_scholes, factor_risk, allocation_stress";
}

}
