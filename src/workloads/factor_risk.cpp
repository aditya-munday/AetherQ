#include "aetherq/workloads/factor_risk.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "aetherq/kernels/dispatch.hpp"
#include "aetherq/measurement/sampler.hpp"
#include "aetherq/runtime/runtime.hpp"

#include "workload_support.hpp"

namespace aetherq {

FactorRiskResult factor_risk_scalar(std::span<const double> returns, std::size_t observations,
                                    std::size_t assets, std::span<const double> weights) {
  FactorRiskResult result;
  std::vector<double> covariance(assets * assets, 0.0);
  scalar_covariance(returns, observations, assets, covariance);

  std::vector<double> covariance_weights(assets, 0.0);
  scalar_matrix_vector(covariance, assets, assets, weights, covariance_weights);

  double variance = 0.0;
  for (std::size_t asset = 0; asset < assets; ++asset) {
    variance += weights[asset] * covariance_weights[asset];
  }
  result.portfolio_variance = variance;
  result.portfolio_volatility = variance > 0.0 ? std::sqrt(variance) : 0.0;

  double marginal_sum = 0.0;
  double component_sum = 0.0;
  const double volatility = result.portfolio_volatility;
  for (std::size_t asset = 0; asset < assets; ++asset) {
    const double marginal = volatility > 0.0 ? covariance_weights[asset] / volatility : 0.0;
    marginal_sum += marginal;
    component_sum += weights[asset] * marginal;
  }
  result.marginal_risk_sum = marginal_sum;
  result.component_risk_sum = component_sum;
  return result;
}

Result<double> factor_risk_reference(std::span<const double> returns, std::size_t observations,
                                     std::size_t assets, std::span<const double> weights) {
  if (observations == 0 || assets == 0) {
    return Error{ErrorCode::invalid_argument, "observations and assets must be positive"};
  }
  return factor_risk_scalar(returns, observations, assets, weights).portfolio_variance;
}

std::string_view FactorRiskWorkload::name() const noexcept {
  return "factor_risk";
}

Result<void> FactorRiskWorkload::run(Runtime& runtime, const WorkloadInputs& inputs,
                                     RunReport& report) {
  const std::size_t assets = inputs.assets;
  const std::size_t observations = inputs.observations;
  if (assets == 0 || observations == 0) {
    return Error{ErrorCode::invalid_argument, "assets and observations must be positive"};
  }

  report = make_report(runtime, "factor_risk", runtime.config().threads);
  report.workload_parameters["assets"] = static_cast<double>(assets);
  report.workload_parameters["observations"] = static_cast<double>(observations);

  std::vector<double> returns(observations * assets, 0.0);
  for (std::size_t row = 0; row < observations; ++row) {
    for (std::size_t asset = 0; asset < assets; ++asset) {
      const double common = 0.01 * std::sin(static_cast<double>(row) * 0.1);
      const double idiosyncratic =
          0.005 * std::cos(static_cast<double>(row * assets + asset) * 0.01);
      returns[row * assets + asset] = common + idiosyncratic;
    }
  }
  std::vector<double> weights(assets, 0.0);
  for (std::size_t asset = 0; asset < assets; ++asset) {
    weights[asset] = 1.0 / static_cast<double>(assets);
  }

  const KernelSet kernels = select_kernels(runtime.config().backend);
  std::vector<double> covariance(assets * assets, 0.0);
  FactorRiskResult outcome;
  auto body = [&kernels, &returns, &weights, &covariance, &outcome, observations, assets] {
    kernels.covariance(returns, observations, assets, covariance);
    std::vector<double> covariance_weights(assets, 0.0);
    kernels.matrix_vector(covariance, assets, assets, weights, covariance_weights);
    double variance = 0.0;
    for (std::size_t asset = 0; asset < assets; ++asset) {
      variance += weights[asset] * covariance_weights[asset];
    }
    outcome.portfolio_variance = variance;
    outcome.portfolio_volatility = variance > 0.0 ? std::sqrt(variance) : 0.0;
    double marginal_sum = 0.0;
    double component_sum = 0.0;
    for (std::size_t asset = 0; asset < assets; ++asset) {
      const double marginal = outcome.portfolio_volatility > 0.0
                                  ? covariance_weights[asset] / outcome.portfolio_volatility
                                  : 0.0;
      marginal_sum += marginal;
      component_sum += weights[asset] * marginal;
    }
    outcome.marginal_risk_sum = marginal_sum;
    outcome.component_risk_sum = component_sum;
  };
  report.timing = collect_samples(runtime.config().measurement, body);

  const FactorRiskResult reference = factor_risk_scalar(returns, observations, assets, weights);
  const double tolerance = std::max(1e-12, std::fabs(reference.portfolio_variance) * 1e-9);
  const double error = std::fabs(outcome.portfolio_variance - reference.portfolio_variance);
  report.correctness.oracle = "scalar_reference_covariance";
  report.correctness.measured = outcome.portfolio_variance;
  report.correctness.expected = reference.portfolio_variance;
  report.correctness.tolerance = tolerance;
  report.correctness.error = error;
  report.correctness.passed = error <= tolerance;
  report.correctness.detail = "portfolio_variance_matches_scalar_reference";

  report.metrics.push_back(
      MetricEntry{"portfolio_variance", outcome.portfolio_variance, "variance"});
  report.metrics.push_back(
      MetricEntry{"portfolio_volatility", outcome.portfolio_volatility, "volatility"});
  report.metrics.push_back(MetricEntry{"component_risk_sum", outcome.component_risk_sum, "risk"});
  report.metrics.push_back(MetricEntry{"elapsed_ms", report.timing.stats.median * 1e-6, "ms"});

  finalize_report(runtime, report);
  return ok_result();
}

}
