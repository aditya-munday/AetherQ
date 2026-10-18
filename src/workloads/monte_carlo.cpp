#include "aetherq/workloads/monte_carlo.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <mutex>

#include "aetherq/executor/parallel_for.hpp"
#include "aetherq/kernels/rng.hpp"
#include "aetherq/measurement/sampler.hpp"
#include "aetherq/runtime/runtime.hpp"

#include "quant_math.hpp"
#include "workload_support.hpp"

namespace aetherq {

namespace {

constexpr std::size_t default_steps = 64;

struct MonteCarloAccumulator {
  double european_sum{0.0};
  double european_square_sum{0.0};
  double asian_sum{0.0};
  double asian_square_sum{0.0};
  std::size_t paths{0};
};

struct PricingParameters {
  double spot{0.0};
  double strike{0.0};
  double rate{0.0};
  double volatility{0.0};
  double maturity{0.0};
  std::size_t steps{0};
  std::uint64_t seed{0};
};

void accumulate_paths(std::size_t begin, std::size_t end, const PricingParameters& parameters,
                      MonteCarloAccumulator& accumulator) {
  const double dt = parameters.maturity / static_cast<double>(parameters.steps);
  const double drift =
      (parameters.rate - (0.5 * parameters.volatility * parameters.volatility)) * dt;
  const double diffusion = parameters.volatility * std::sqrt(dt);
  const double discount = std::exp(-parameters.rate * parameters.maturity);
  const double steps_as_double = static_cast<double>(parameters.steps);

  for (std::size_t path = begin; path < end; ++path) {
    Philox4x32 generator(parameters.seed, static_cast<std::uint64_t>(path) + 1);
    double price = parameters.spot;
    double running_sum = 0.0;
    for (std::size_t step = 0; step < parameters.steps; ++step) {
      price *= std::exp(drift + (diffusion * generator.next_normal()));
      running_sum += price;
    }
    const double european_payoff =
        price > parameters.strike ? (price - parameters.strike) * discount : 0.0;
    const double asian_price = running_sum / steps_as_double;
    const double asian_payoff =
        asian_price > parameters.strike ? (asian_price - parameters.strike) * discount : 0.0;
    accumulator.european_sum += european_payoff;
    accumulator.european_square_sum += european_payoff * european_payoff;
    accumulator.asian_sum += asian_payoff;
    accumulator.asian_square_sum += asian_payoff * asian_payoff;
    accumulator.paths += 1;
  }
}

double standard_error(double sum, double square_sum, std::size_t count) {
  if (count == 0) {
    return 0.0;
  }
  const double n = static_cast<double>(count);
  const double mean = sum / n;
  const double variance = (square_sum / n) - (mean * mean);
  return std::sqrt(variance > 0.0 ? variance / n : 0.0);
}

}

double normal_cdf_scalar(double x) noexcept {
  return standard_normal_cdf(x);
}

double black_scholes_call(double spot, double strike, double rate, double volatility,
                          double maturity, double dividend_yield) noexcept {
  const double sqrt_t = std::sqrt(maturity);
  const double d1 = (std::log(spot / strike) +
                     ((rate - dividend_yield) + (0.5 * volatility * volatility)) * maturity) /
                    (volatility * sqrt_t);
  const double d2 = d1 - (volatility * sqrt_t);
  return (spot * std::exp(-dividend_yield * maturity) * standard_normal_cdf(d1)) -
         (strike * std::exp(-rate * maturity) * standard_normal_cdf(d2));
}

double black_scholes_put(double spot, double strike, double rate, double volatility,
                         double maturity, double dividend_yield) noexcept {
  const double sqrt_t = std::sqrt(maturity);
  const double d1 = (std::log(spot / strike) +
                     ((rate - dividend_yield) + (0.5 * volatility * volatility)) * maturity) /
                    (volatility * sqrt_t);
  const double d2 = d1 - (volatility * sqrt_t);
  return (strike * std::exp(-rate * maturity) * standard_normal_cdf(-d2)) -
         (spot * std::exp(-dividend_yield * maturity) * standard_normal_cdf(-d1));
}

Result<MonteCarloResult> monte_carlo_price(Runtime& runtime, std::size_t paths, std::size_t steps,
                                           double spot, double strike, double rate,
                                           double volatility, double maturity, std::uint64_t seed) {
  if (paths == 0) {
    return Error{ErrorCode::invalid_argument, "paths must be at least 1"};
  }
  if (steps == 0) {
    return Error{ErrorCode::invalid_argument, "steps must be at least 1"};
  }
  if (volatility <= 0.0 || maturity <= 0.0 || spot <= 0.0 || strike <= 0.0) {
    return Error{ErrorCode::invalid_argument, "pricing inputs must be positive"};
  }

  PricingParameters parameters;
  parameters.spot = spot;
  parameters.strike = strike;
  parameters.rate = rate;
  parameters.volatility = volatility;
  parameters.maturity = maturity;
  parameters.steps = steps;
  parameters.seed = seed;

  MonteCarloAccumulator accumulator;
  std::mutex accumulator_mutex;
  auto body = [&accumulator, &accumulator_mutex, &parameters](std::size_t begin, std::size_t end) {
    MonteCarloAccumulator local;
    accumulate_paths(begin, end, parameters, local);
    const std::lock_guard<std::mutex> lock(accumulator_mutex);
    accumulator.european_sum += local.european_sum;
    accumulator.european_square_sum += local.european_square_sum;
    accumulator.asian_sum += local.asian_sum;
    accumulator.asian_square_sum += local.asian_square_sum;
    accumulator.paths += local.paths;
  };
  auto submitted = parallel_for(runtime.executor(), paths, runtime.config().parallel_for,
                                runtime.config().chunk_grain, body);
  if (!submitted) {
    return submitted.error();
  }

  MonteCarloResult result;
  result.paths = paths;
  result.steps = steps;
  const double count = static_cast<double>(paths);
  result.european_price = accumulator.european_sum / count;
  result.asian_price = accumulator.asian_sum / count;
  result.european_standard_error =
      standard_error(accumulator.european_sum, accumulator.european_square_sum, paths);
  result.asian_standard_error =
      standard_error(accumulator.asian_sum, accumulator.asian_square_sum, paths);
  return result;
}

std::string_view MonteCarloWorkload::name() const noexcept {
  return "monte_carlo";
}

Result<void> MonteCarloWorkload::run(Runtime& runtime, const WorkloadInputs& inputs,
                                     RunReport& report) {
  const double spot = 100.0;
  const double strike = 100.0;
  const double rate = 0.05;
  const double volatility = 0.2;
  const double maturity = 1.0;
  const std::size_t steps = default_steps;

  report = make_report(runtime, "monte_carlo", runtime.config().threads);
  report.workload_parameters["paths"] = static_cast<double>(inputs.paths);
  report.workload_parameters["steps"] = static_cast<double>(steps);
  report.workload_parameters["spot"] = spot;
  report.workload_parameters["strike"] = strike;
  report.workload_parameters["rate"] = rate;
  report.workload_parameters["volatility"] = volatility;
  report.workload_parameters["maturity"] = maturity;

  Result<MonteCarloResult> outcome = Error{ErrorCode::internal, "not executed"};
  auto body = [&runtime, &inputs, &outcome, spot, strike, rate, volatility, maturity] {
    outcome = monte_carlo_price(runtime, inputs.paths, steps, spot, strike, rate, volatility,
                                maturity, runtime.config().seed);
  };
  report.timing = collect_samples(runtime.config().measurement, body);
  if (!outcome) {
    return outcome.error();
  }
  const MonteCarloResult& result = outcome.value();

  const double analytic = black_scholes_call(spot, strike, rate, volatility, maturity, 0.0);
  const double tolerance = 6.0 * std::max(result.european_standard_error, 1e-12);
  const double error = std::fabs(result.european_price - analytic);
  report.correctness.oracle = "black_scholes_closed_form";
  report.correctness.measured = result.european_price;
  report.correctness.expected = analytic;
  report.correctness.tolerance = tolerance;
  report.correctness.error = error;
  report.correctness.passed = error <= tolerance;
  report.correctness.detail = "six_standard_error_confidence_band";

  report.metrics.push_back(MetricEntry{"european_price", result.european_price, "currency"});
  report.metrics.push_back(
      MetricEntry{"european_standard_error", result.european_standard_error, "currency"});
  report.metrics.push_back(MetricEntry{"asian_price", result.asian_price, "currency"});
  report.metrics.push_back(
      MetricEntry{"asian_standard_error", result.asian_standard_error, "currency"});
  report.metrics.push_back(MetricEntry{"black_scholes_reference", analytic, "currency"});
  report.metrics.push_back(MetricEntry{
      "paths_per_second", static_cast<double>(inputs.paths) / (report.timing.stats.median * 1e-9),
      "paths/s"});
  report.metrics.push_back(MetricEntry{"elapsed_ms", report.timing.stats.median * 1e-6, "ms"});

  finalize_report(runtime, report);
  return ok_result();
}

}
