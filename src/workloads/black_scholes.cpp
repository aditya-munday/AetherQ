#include "aetherq/workloads/black_scholes.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "aetherq/executor/parallel_for.hpp"
#include "aetherq/kernels/dispatch.hpp"
#include "aetherq/measurement/sampler.hpp"
#include "aetherq/runtime/runtime.hpp"
#include "aetherq/workloads/monte_carlo.hpp"

#include "quant_math.hpp"
#include "workload_support.hpp"

namespace aetherq {

namespace {

void greeks_scalar(double spot, double strike, double rate, double volatility, double maturity,
                   double dividend_yield, Greeks& greeks) noexcept {
  const double sqrt_t = std::sqrt(maturity);
  const double d1 = (std::log(spot / strike) +
                     ((rate - dividend_yield) + (0.5 * volatility * volatility)) * maturity) /
                    (volatility * sqrt_t);
  const double d2 = d1 - (volatility * sqrt_t);
  const double discount_q = std::exp(-dividend_yield * maturity);
  const double discount_r = std::exp(-rate * maturity);
  const double pdf_d1 = standard_normal_pdf(d1);
  greeks.delta = discount_q * standard_normal_cdf(d1);
  greeks.gamma = (discount_q * pdf_d1) / (spot * volatility * sqrt_t);
  greeks.vega = spot * discount_q * pdf_d1 * sqrt_t;
  greeks.theta = ((-(spot * discount_q * pdf_d1 * volatility) / (2.0 * sqrt_t)) -
                  (rate * strike * discount_r * standard_normal_cdf(d2)) +
                  (dividend_yield * spot * discount_q * standard_normal_cdf(d1)));
  greeks.rho = strike * maturity * discount_r * standard_normal_cdf(d2);
}

}

void black_scholes_greeks_scalar(double spot, double strike, double rate, double volatility,
                                 double maturity, double dividend_yield, Greeks& greeks) noexcept {
  greeks_scalar(spot, strike, rate, volatility, maturity, dividend_yield, greeks);
}

void black_scholes_price_batch_scalar(const OptionBatch& batch, std::span<double> prices) noexcept {
  const std::size_t count = prices.size();
  for (std::size_t i = 0; i < count; ++i) {
    prices[i] = black_scholes_call(batch.spot[i], batch.strike[i], batch.rate[i],
                                   batch.volatility[i], batch.maturity[i], batch.dividend_yield[i]);
  }
}

void black_scholes_greeks_batch_scalar(const OptionBatch& batch,
                                       std::span<Greeks> greeks) noexcept {
  const std::size_t count = greeks.size();
  for (std::size_t i = 0; i < count; ++i) {
    greeks_scalar(batch.spot[i], batch.strike[i], batch.rate[i], batch.volatility[i],
                  batch.maturity[i], batch.dividend_yield[i], greeks[i]);
  }
}

Result<double> black_scholes_price_reference(double spot, double strike, double rate,
                                             double volatility, double maturity,
                                             double dividend_yield) {
  if (spot <= 0.0 || strike <= 0.0 || volatility <= 0.0 || maturity <= 0.0) {
    return Error{ErrorCode::invalid_argument, "reference pricing inputs must be positive"};
  }
  return black_scholes_call(spot, strike, rate, volatility, maturity, dividend_yield);
}

std::string_view BlackScholesWorkload::name() const noexcept {
  return "black_scholes";
}

Result<void> BlackScholesWorkload::run(Runtime& runtime, const WorkloadInputs& inputs,
                                       RunReport& report) {
  const std::size_t count = inputs.options;
  if (count == 0) {
    return Error{ErrorCode::invalid_argument, "options must be at least 1"};
  }

  report = make_report(runtime, "black_scholes", runtime.config().threads);
  report.workload_parameters["options"] = static_cast<double>(count);

  std::vector<double> spot(count);
  std::vector<double> strike(count);
  std::vector<double> rate(count);
  std::vector<double> volatility(count);
  std::vector<double> maturity(count);
  std::vector<double> dividend(count);
  for (std::size_t i = 0; i < count; ++i) {
    const double fraction = static_cast<double>(i) / static_cast<double>(count);
    spot[i] = 50.0 + (100.0 * fraction);
    strike[i] = 50.0 + (100.0 * (1.0 - fraction));
    rate[i] = 0.01 + (0.05 * fraction);
    volatility[i] = 0.1 + (0.4 * fraction);
    maturity[i] = 0.1 + (2.0 * fraction);
    dividend[i] = 0.02 * fraction;
  }
  OptionBatch batch{spot, strike, rate, volatility, maturity, dividend};

  std::vector<double> prices(count, 0.0);
  std::vector<Greeks> greeks(count);

  auto body = [&runtime, &batch, &prices, &greeks, count] {
    auto submitted = parallel_for(
        runtime.executor(), count, runtime.config().parallel_for, runtime.config().chunk_grain,
        [&batch, &prices, &greeks](std::size_t begin, std::size_t end) {
          for (std::size_t i = begin; i < end; ++i) {
            prices[i] =
                black_scholes_call(batch.spot[i], batch.strike[i], batch.rate[i],
                                   batch.volatility[i], batch.maturity[i], batch.dividend_yield[i]);
            greeks_scalar(batch.spot[i], batch.strike[i], batch.rate[i], batch.volatility[i],
                          batch.maturity[i], batch.dividend_yield[i], greeks[i]);
          }
        });
    static_cast<void>(submitted);
  };
  report.timing = collect_samples(runtime.config().measurement, body);

  double max_price_error = 0.0;
  double max_parity_error = 0.0;
  double gamma_sum = 0.0;
  double vega_sum = 0.0;
  for (std::size_t i = 0; i < count; ++i) {
    const double reference =
        black_scholes_call(spot[i], strike[i], rate[i], volatility[i], maturity[i], dividend[i]);
    max_price_error = std::max(max_price_error, std::fabs(prices[i] - reference));
    const double put =
        black_scholes_put(spot[i], strike[i], rate[i], volatility[i], maturity[i], dividend[i]);
    const double parity_lhs = prices[i] - put;
    const double parity_rhs = (spot[i] * std::exp(-dividend[i] * maturity[i])) -
                              (strike[i] * std::exp(-rate[i] * maturity[i]));
    max_parity_error = std::max(max_parity_error, std::fabs(parity_lhs - parity_rhs));
    gamma_sum += greeks[i].gamma;
    vega_sum += greeks[i].vega;
  }

  const double price_tolerance = 1e-9;
  report.correctness.oracle = "closed_form_scalar_reference";
  report.correctness.measured = max_price_error;
  report.correctness.expected = 0.0;
  report.correctness.tolerance = price_tolerance;
  report.correctness.error = max_price_error;
  report.correctness.passed = max_price_error <= price_tolerance && max_parity_error <= 1e-6;
  report.correctness.detail = "price_and_put_call_parity";

  report.metrics.push_back(MetricEntry{"max_price_error", max_price_error, "currency"});
  report.metrics.push_back(MetricEntry{"max_parity_error", max_parity_error, "currency"});
  report.metrics.push_back(MetricEntry{"gamma_sum", gamma_sum, "1/currency"});
  report.metrics.push_back(MetricEntry{"vega_sum", vega_sum, "currency"});
  report.metrics.push_back(
      MetricEntry{"options_per_second",
                  static_cast<double>(count) / (report.timing.stats.median * 1e-9), "options/s"});
  report.metrics.push_back(MetricEntry{"elapsed_ms", report.timing.stats.median * 1e-6, "ms"});

  finalize_report(runtime, report);
  return ok_result();
}

}
