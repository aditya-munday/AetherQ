#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "aetherq/kernels/dispatch.hpp"
#include "aetherq/kernels/kernels.hpp"
#include "aetherq/kernels/rng.hpp"
#include "aetherq/workloads/monte_carlo.hpp"

using namespace aetherq;

namespace {

std::vector<double> random_values(std::size_t count, std::uint64_t seed) {
  std::vector<double> values(count, 0.0);
  Philox4x32::fill_uniform(seed, 1, 0, values);
  return values;
}

}

TEST(Invariants, WeightedSumIsLinearInWeights) {
  const std::size_t assets = 8;
  const std::size_t observations = 32;
  const auto values = random_values(observations * assets, 1);
  std::vector<double> weights(assets);
  for (std::size_t i = 0; i < assets; ++i) {
    weights[i] = 0.1 * static_cast<double>(i + 1);
  }
  std::vector<double> doubled(assets);
  for (std::size_t i = 0; i < assets; ++i) {
    doubled[i] = weights[i] * 2.0;
  }
  std::vector<double> base_output(observations, 0.0);
  std::vector<double> doubled_output(observations, 0.0);
  scalar_weighted_sum(values, weights, base_output);
  scalar_weighted_sum(values, doubled, doubled_output);
  for (std::size_t i = 0; i < observations; ++i) {
    EXPECT_NEAR(doubled_output[i], 2.0 * base_output[i], 1e-9);
  }
}

TEST(Invariants, WeightedSumIsAdditiveInValues) {
  const std::size_t assets = 4;
  const std::size_t observations = 16;
  const auto lhs = random_values(observations * assets, 2);
  const auto rhs = random_values(observations * assets, 3);
  std::vector<double> combined(lhs.size());
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    combined[i] = lhs[i] + rhs[i];
  }
  std::vector<double> weights(assets, 0.25);
  std::vector<double> lhs_output(observations, 0.0);
  std::vector<double> rhs_output(observations, 0.0);
  std::vector<double> combined_output(observations, 0.0);
  scalar_weighted_sum(lhs, weights, lhs_output);
  scalar_weighted_sum(rhs, weights, rhs_output);
  scalar_weighted_sum(combined, weights, combined_output);
  for (std::size_t i = 0; i < observations; ++i) {
    EXPECT_NEAR(combined_output[i], lhs_output[i] + rhs_output[i], 1e-9);
  }
}

TEST(Invariants, PutCallParityHolds) {
  for (const double volatility : {0.1, 0.2, 0.5}) {
    for (const double maturity : {0.5, 1.0, 2.0}) {
      const double spot = 100.0;
      const double strike = 95.0;
      const double rate = 0.03;
      const double dividend = 0.01;
      const double call = black_scholes_call(spot, strike, rate, volatility, maturity, dividend);
      const double put = black_scholes_put(spot, strike, rate, volatility, maturity, dividend);
      const double parity =
          (spot * std::exp(-dividend * maturity)) - (strike * std::exp(-rate * maturity));
      EXPECT_NEAR(call - put, parity, 1e-10)
          << "volatility=" << volatility << " maturity=" << maturity;
    }
  }
}

TEST(Invariants, CallPriceIncreasesWithVolatility) {
  double previous = -1.0;
  for (const double volatility : {0.05, 0.1, 0.2, 0.4, 0.8}) {
    const double price = black_scholes_call(100.0, 100.0, 0.05, volatility, 1.0, 0.0);
    EXPECT_GT(price, previous) << "volatility=" << volatility;
    previous = price;
  }
}

TEST(Invariants, CallPriceIncreasesWithSpot) {
  double previous = -1.0;
  for (const double spot : {50.0, 80.0, 100.0, 120.0, 200.0}) {
    const double price = black_scholes_call(spot, 100.0, 0.05, 0.2, 1.0, 0.0);
    EXPECT_GT(price, previous) << "spot=" << spot;
    previous = price;
  }
}

TEST(Invariants, CallPriceDecreasesWithStrike) {
  double previous = 1e9;
  for (const double strike : {50.0, 80.0, 100.0, 120.0, 200.0}) {
    const double price = black_scholes_call(100.0, strike, 0.05, 0.2, 1.0, 0.0);
    EXPECT_LT(price, previous) << "strike=" << strike;
    previous = price;
  }
}

TEST(Invariants, NormalCdfIsMonotone) {
  double previous = -1.0;
  for (double x = -5.0; x <= 5.0; x += 0.1) {
    const double value = normal_cdf_scalar(x);
    EXPECT_GE(value, previous);
    previous = value;
  }
}

TEST(Invariants, NormalCdfBounds) {
  EXPECT_NEAR(normal_cdf_scalar(0.0), 0.5, 1e-12);
  EXPECT_NEAR(normal_cdf_scalar(-40.0), 0.0, 1e-12);
  EXPECT_NEAR(normal_cdf_scalar(40.0), 1.0, 1e-12);
}
