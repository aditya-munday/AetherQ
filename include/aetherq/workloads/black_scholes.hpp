#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "aetherq/runtime/result.hpp"
#include "aetherq/workloads/workload.hpp"

namespace aetherq {

struct OptionBatch {
  std::span<const double> spot;
  std::span<const double> strike;
  std::span<const double> rate;
  std::span<const double> volatility;
  std::span<const double> maturity;
  std::span<const double> dividend_yield;
};

struct Greeks {
  double delta{0.0};
  double gamma{0.0};
  double vega{0.0};
  double theta{0.0};
  double rho{0.0};
};

void black_scholes_greeks_scalar(double spot, double strike, double rate, double volatility,
                                 double maturity, double dividend_yield, Greeks& greeks) noexcept;

void black_scholes_price_batch_scalar(const OptionBatch& batch, std::span<double> prices) noexcept;
void black_scholes_greeks_batch_scalar(const OptionBatch& batch, std::span<Greeks> greeks) noexcept;

Result<double> black_scholes_price_reference(double spot, double strike, double rate,
                                             double volatility, double maturity,
                                             double dividend_yield);

class BlackScholesWorkload final : public Workload {
public:
  [[nodiscard]] std::string_view name() const noexcept override;
  Result<void> run(Runtime& runtime, const WorkloadInputs& inputs, RunReport& report) override;
};

}
