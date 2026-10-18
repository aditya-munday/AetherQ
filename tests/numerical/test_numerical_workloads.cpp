#include <cmath>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "aetherq/runtime/runtime.hpp"
#include "aetherq/workloads/black_scholes.hpp"
#include "aetherq/workloads/factor_risk.hpp"
#include "aetherq/workloads/monte_carlo.hpp"

using namespace aetherq;

namespace {

std::unique_ptr<Runtime> make_runtime(std::size_t threads, Backend backend) {
  RuntimeConfig config = default_config();
  config.threads = threads;
  config.backend = backend;
  config.scheduler = threads > 1 ? SchedulerPolicy::thread_pool : SchedulerPolicy::serial;
  config.measurement.repetitions = 2;
  config.measurement.warmup_iterations = 1;
  config.measurement.min_samples = 1;
  auto runtime = Runtime::create(config, default_logger());
  return runtime.ok() ? std::move(runtime.value()) : nullptr;
}

}

TEST(MonteCarlo, EuropeanPriceMatchesClosedForm) {
  auto runtime = make_runtime(1, Backend::scalar);
  ASSERT_NE(runtime, nullptr);
  auto result = monte_carlo_price(*runtime, 200000, 64, 100.0, 100.0, 0.05, 0.2, 1.0, 20260101);
  ASSERT_TRUE(result.ok());
  const double analytic = black_scholes_call(100.0, 100.0, 0.05, 0.2, 1.0, 0.0);
  const double error = std::fabs(result.value().european_price - analytic);
  EXPECT_LE(error, 6.0 * result.value().european_standard_error + 1e-6);
}

TEST(MonteCarlo, AgreesAcrossThreadCountsWithinReductionTolerance) {
  auto single = make_runtime(1, Backend::scalar);
  auto multi = make_runtime(4, Backend::scalar);
  ASSERT_NE(single, nullptr);
  ASSERT_NE(multi, nullptr);
  auto single_result = monte_carlo_price(*single, 50000, 32, 100.0, 100.0, 0.05, 0.2, 1.0, 777);
  auto multi_result = monte_carlo_price(*multi, 50000, 32, 100.0, 100.0, 0.05, 0.2, 1.0, 777);
  ASSERT_TRUE(single_result.ok());
  ASSERT_TRUE(multi_result.ok());
  const double european_reference = single_result.value().european_price;
  const double asian_reference = single_result.value().asian_price;
  EXPECT_NEAR(multi_result.value().european_price, european_reference,
              std::fabs(european_reference) * 1e-12);
  EXPECT_NEAR(multi_result.value().asian_price, asian_reference,
              std::fabs(asian_reference) * 1e-12);
}

TEST(MonteCarlo, RejectsInvalidInputs) {
  auto runtime = make_runtime(1, Backend::scalar);
  ASSERT_NE(runtime, nullptr);
  EXPECT_FALSE(monte_carlo_price(*runtime, 0, 10, 100.0, 100.0, 0.05, 0.2, 1.0, 1).ok());
  EXPECT_FALSE(monte_carlo_price(*runtime, 100, 0, 100.0, 100.0, 0.05, 0.2, 1.0, 1).ok());
  EXPECT_FALSE(monte_carlo_price(*runtime, 100, 10, 100.0, 100.0, 0.05, -0.2, 1.0, 1).ok());
}

TEST(BlackScholes, MatchesKnownTextbookValue) {
  const double price = black_scholes_call(100.0, 100.0, 0.05, 0.2, 1.0, 0.0);
  EXPECT_NEAR(price, 10.4506, 1e-3);
}

TEST(BlackScholes, BatchPricesMatchScalarReference) {
  const std::size_t count = 500;
  std::vector<double> spot(count);
  std::vector<double> strike(count);
  std::vector<double> rate(count);
  std::vector<double> volatility(count);
  std::vector<double> maturity(count);
  std::vector<double> dividend(count);
  for (std::size_t i = 0; i < count; ++i) {
    const double fraction = static_cast<double>(i) / static_cast<double>(count);
    spot[i] = 80.0 + (40.0 * fraction);
    strike[i] = 80.0 + (40.0 * (1.0 - fraction));
    rate[i] = 0.02 + (0.04 * fraction);
    volatility[i] = 0.15 + (0.3 * fraction);
    maturity[i] = 0.25 + (1.5 * fraction);
    dividend[i] = 0.01 * fraction;
  }
  OptionBatch batch{spot, strike, rate, volatility, maturity, dividend};
  std::vector<double> prices(count, 0.0);
  black_scholes_price_batch_scalar(batch, prices);
  for (std::size_t i = 0; i < count; ++i) {
    const double reference =
        black_scholes_call(spot[i], strike[i], rate[i], volatility[i], maturity[i], dividend[i]);
    EXPECT_NEAR(prices[i], reference, 1e-12);
  }
}

TEST(BlackScholes, GreeksMatchFiniteDifferences) {
  const double spot = 100.0;
  const double strike = 100.0;
  const double rate = 0.05;
  const double volatility = 0.2;
  const double maturity = 1.0;
  const double dividend = 0.0;
  Greeks greeks;
  black_scholes_greeks_scalar(spot, strike, rate, volatility, maturity, dividend, greeks);

  const double bump = 1e-4;
  const double delta_numeric =
      (black_scholes_call(spot + bump, strike, rate, volatility, maturity, dividend) -
       black_scholes_call(spot - bump, strike, rate, volatility, maturity, dividend)) /
      (2.0 * bump);
  EXPECT_NEAR(greeks.delta, delta_numeric, 1e-6);

  const double vega_numeric =
      (black_scholes_call(spot, strike, rate, volatility + bump, maturity, dividend) -
       black_scholes_call(spot, strike, rate, volatility - bump, maturity, dividend)) /
      (2.0 * bump);
  EXPECT_NEAR(greeks.vega, vega_numeric, 1e-4);

  const double gamma_numeric =
      (black_scholes_call(spot + bump, strike, rate, volatility, maturity, dividend) -
       (2.0 * black_scholes_call(spot, strike, rate, volatility, maturity, dividend)) +
       black_scholes_call(spot - bump, strike, rate, volatility, maturity, dividend)) /
      (bump * bump);
  EXPECT_NEAR(greeks.gamma, gamma_numeric, 1e-4);
}

TEST(BlackScholes, ReferenceRejectsInvalidInputs) {
  EXPECT_FALSE(black_scholes_price_reference(-1.0, 100.0, 0.05, 0.2, 1.0, 0.0).ok());
  EXPECT_FALSE(black_scholes_price_reference(100.0, 100.0, 0.05, 0.0, 1.0, 0.0).ok());
}

TEST(FactorRisk, PortfolioVarianceMatchesScalarReference) {
  const std::size_t observations = 512;
  const std::size_t assets = 16;
  std::vector<double> returns(observations * assets);
  for (std::size_t row = 0; row < observations; ++row) {
    for (std::size_t asset = 0; asset < assets; ++asset) {
      returns[row * assets + asset] =
          0.01 * std::sin(static_cast<double>(row) * 0.05) +
          0.005 * std::cos(static_cast<double>(row * assets + asset) * 0.02);
    }
  }
  std::vector<double> weights(assets, 1.0 / static_cast<double>(assets));
  const FactorRiskResult result = factor_risk_scalar(returns, observations, assets, weights);
  const double reference = factor_risk_reference(returns, observations, assets, weights).value();
  EXPECT_NEAR(result.portfolio_variance, reference, 1e-15);
  EXPECT_GE(result.portfolio_variance, 0.0);
}

TEST(FactorRisk, ComponentRiskSumsToVolatility) {
  const std::size_t observations = 256;
  const std::size_t assets = 8;
  std::vector<double> returns(observations * assets);
  for (std::size_t i = 0; i < returns.size(); ++i) {
    returns[i] = 0.02 * std::sin(static_cast<double>(i) * 0.11);
  }
  std::vector<double> weights(assets, 1.0 / static_cast<double>(assets));
  const FactorRiskResult result = factor_risk_scalar(returns, observations, assets, weights);
  EXPECT_NEAR(result.component_risk_sum, result.portfolio_volatility, 1e-9);
}

TEST(FactorRisk, RejectsEmptyInputs) {
  std::vector<double> returns(10, 0.0);
  std::vector<double> weights(2, 0.5);
  EXPECT_FALSE(factor_risk_reference(returns, 0, 2, weights).ok());
  EXPECT_FALSE(factor_risk_reference(returns, 5, 0, weights).ok());
}
