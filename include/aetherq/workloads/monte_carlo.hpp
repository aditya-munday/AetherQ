#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "aetherq/runtime/result.hpp"
#include "aetherq/workloads/workload.hpp"

namespace aetherq {

struct MonteCarloResult {
  double european_price{0.0};
  double european_standard_error{0.0};
  double asian_price{0.0};
  double asian_standard_error{0.0};
  std::size_t paths{0};
  std::size_t steps{0};
};

double black_scholes_call(double spot, double strike, double rate, double volatility,
                          double maturity, double dividend_yield) noexcept;
double black_scholes_put(double spot, double strike, double rate, double volatility,
                         double maturity, double dividend_yield) noexcept;
double normal_cdf_scalar(double x) noexcept;

Result<MonteCarloResult> monte_carlo_price(Runtime& runtime, std::size_t paths, std::size_t steps,
                                           double spot, double strike, double rate,
                                           double volatility, double maturity, std::uint64_t seed);

class MonteCarloWorkload final : public Workload {
public:
  [[nodiscard]] std::string_view name() const noexcept override;
  Result<void> run(Runtime& runtime, const WorkloadInputs& inputs, RunReport& report) override;
};

}
