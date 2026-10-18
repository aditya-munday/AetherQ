#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "aetherq/kernels/dispatch.hpp"
#include "aetherq/kernels/kernels.hpp"
#include "aetherq/kernels/rng.hpp"

using namespace aetherq;

namespace {

std::vector<double> make_sequence(std::size_t count, double scale) {
  std::vector<double> values(count);
  for (std::size_t i = 0; i < count; ++i) {
    values[i] = scale * static_cast<double>(i + 1);
  }
  return values;
}

}

TEST(Kernels, DotMatchesScalarAndOptimized) {
  const auto lhs = make_sequence(257, 0.5);
  const auto rhs = make_sequence(257, 0.25);
  EXPECT_NEAR(scalar_dot(lhs, rhs), optimized_dot(lhs, rhs), 1e-9);
}

TEST(Kernels, SumMatchesForNonVectorWidthSizes) {
  for (const std::size_t size : {0U, 1U, 3U, 7U, 15U, 16U, 63U, 64U, 65U, 1000U}) {
    const auto values = make_sequence(size, 1.0);
    EXPECT_NEAR(scalar_sum(values), optimized_sum(values), 1e-9) << "size=" << size;
  }
}

TEST(Kernels, NormMatchesForNonVectorWidthSizes) {
  for (const std::size_t size : {1U, 3U, 7U, 16U, 33U, 129U}) {
    const auto values = make_sequence(size, 1.0);
    EXPECT_NEAR(scalar_norm(values), optimized_norm(values), 1e-9) << "size=" << size;
  }
}

TEST(Kernels, ScaleMatchesForNonVectorWidthSizes) {
  for (const std::size_t size : {1U, 3U, 7U, 16U, 33U, 129U}) {
    const auto input = make_sequence(size, 2.0);
    std::vector<double> scalar_output(size, 0.0);
    std::vector<double> optimized_output(size, 0.0);
    scalar_scale(1.5, input, scalar_output);
    optimized_scale(1.5, input, optimized_output);
    for (std::size_t i = 0; i < size; ++i) {
      EXPECT_NEAR(scalar_output[i], optimized_output[i], 1e-12);
    }
  }
}

TEST(Kernels, AxpyMatchesForNonVectorWidthSizes) {
  for (const std::size_t size : {1U, 3U, 7U, 16U, 33U, 129U}) {
    const auto x = make_sequence(size, 1.0);
    std::vector<double> scalar_y(size, 1.0);
    std::vector<double> optimized_y(size, 1.0);
    scalar_axpy(2.0, x, scalar_y);
    optimized_axpy(2.0, x, optimized_y);
    for (std::size_t i = 0; i < size; ++i) {
      EXPECT_NEAR(scalar_y[i], optimized_y[i], 1e-12);
    }
  }
}

TEST(Kernels, MatrixVectorMatches) {
  const std::size_t rows = 17;
  const std::size_t cols = 13;
  std::vector<double> matrix(rows * cols);
  for (std::size_t i = 0; i < matrix.size(); ++i) {
    matrix[i] = static_cast<double>(i % 7) * 0.3;
  }
  const auto vector = make_sequence(cols, 0.5);
  std::vector<double> scalar_output(rows, 0.0);
  std::vector<double> optimized_output(rows, 0.0);
  scalar_matrix_vector(matrix, rows, cols, vector, scalar_output);
  optimized_matrix_vector(matrix, rows, cols, vector, optimized_output);
  for (std::size_t i = 0; i < rows; ++i) {
    EXPECT_NEAR(scalar_output[i], optimized_output[i], 1e-9);
  }
}

TEST(Kernels, MatrixMatrixMatches) {
  const std::size_t lhs_rows = 9;
  const std::size_t shared = 11;
  const std::size_t rhs_cols = 13;
  std::vector<double> lhs(lhs_rows * shared);
  std::vector<double> rhs(shared * rhs_cols);
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    lhs[i] = static_cast<double>(i % 5) * 0.2;
  }
  for (std::size_t i = 0; i < rhs.size(); ++i) {
    rhs[i] = static_cast<double>(i % 3) * 0.4;
  }
  std::vector<double> scalar_output(lhs_rows * rhs_cols, 0.0);
  std::vector<double> optimized_output(lhs_rows * rhs_cols, 0.0);
  scalar_matrix_matrix(lhs, lhs_rows, shared, rhs, rhs_cols, scalar_output);
  optimized_matrix_matrix(lhs, lhs_rows, shared, rhs, rhs_cols, optimized_output);
  for (std::size_t i = 0; i < scalar_output.size(); ++i) {
    EXPECT_NEAR(scalar_output[i], optimized_output[i], 1e-9);
  }
}

TEST(Kernels, CovarianceMatches) {
  const std::size_t observations = 64;
  const std::size_t assets = 5;
  std::vector<double> returns(observations * assets);
  for (std::size_t i = 0; i < returns.size(); ++i) {
    returns[i] = std::sin(static_cast<double>(i) * 0.1);
  }
  std::vector<double> scalar_output(assets * assets, 0.0);
  std::vector<double> optimized_output(assets * assets, 0.0);
  scalar_covariance(returns, observations, assets, scalar_output);
  optimized_covariance(returns, observations, assets, optimized_output);
  for (std::size_t i = 0; i < scalar_output.size(); ++i) {
    EXPECT_NEAR(scalar_output[i], optimized_output[i], 1e-9);
  }
}

TEST(Kernels, WeightedSumMatches) {
  const std::size_t assets = 6;
  const std::size_t observations = 20;
  std::vector<double> values(observations * assets);
  for (std::size_t i = 0; i < values.size(); ++i) {
    values[i] = static_cast<double>(i % 11) * 0.1;
  }
  const auto weights = make_sequence(assets, 0.05);
  std::vector<double> scalar_output(observations, 0.0);
  std::vector<double> optimized_output(observations, 0.0);
  scalar_weighted_sum(values, weights, scalar_output);
  optimized_weighted_sum(values, weights, optimized_output);
  for (std::size_t i = 0; i < observations; ++i) {
    EXPECT_NEAR(scalar_output[i], optimized_output[i], 1e-12);
  }
}

TEST(Kernels, VectorTranscendentalsMatch) {
  const std::size_t size = 129;
  std::vector<double> input(size);
  for (std::size_t i = 0; i < size; ++i) {
    input[i] = 0.01 + (static_cast<double>(i) * 0.05);
  }
  std::vector<double> scalar_output(size, 0.0);
  std::vector<double> optimized_output(size, 0.0);

  scalar_exp(input, scalar_output);
  optimized_exp(input, optimized_output);
  for (std::size_t i = 0; i < size; ++i) {
    EXPECT_NEAR(scalar_output[i], optimized_output[i], 1e-12);
  }

  scalar_log(input, scalar_output);
  optimized_log(input, optimized_output);
  for (std::size_t i = 0; i < size; ++i) {
    EXPECT_NEAR(scalar_output[i], optimized_output[i], 1e-12);
  }

  scalar_normal_cdf(input, scalar_output);
  optimized_normal_cdf(input, optimized_output);
  for (std::size_t i = 0; i < size; ++i) {
    EXPECT_NEAR(scalar_output[i], optimized_output[i], 1e-12);
  }
}

TEST(Kernels, NanAndInfPropagate) {
  const std::vector<double> values{1.0, std::numeric_limits<double>::quiet_NaN(), 3.0};
  EXPECT_TRUE(std::isnan(scalar_sum(values)));
  EXPECT_TRUE(std::isnan(optimized_sum(values)));

  const std::vector<double> infinite{1.0, std::numeric_limits<double>::infinity()};
  EXPECT_TRUE(std::isinf(scalar_sum(infinite)));
  EXPECT_TRUE(std::isinf(optimized_sum(infinite)));
}

TEST(Dispatch, SelectsRequestedBackend) {
  const KernelSet scalar_set = select_kernels(Backend::scalar);
  const KernelSet optimized_set = select_kernels(Backend::optimized);
  EXPECT_EQ(scalar_set.sum, &scalar_sum);
  EXPECT_EQ(optimized_set.sum, &optimized_sum);
}

TEST(Dispatch, DetectsFeatures) {
  const CpuFeatures features = detect_cpu_features();
  const std::string_view name = best_backend_name(features);
  EXPECT_FALSE(name.empty());
}

TEST(Rng, IsDeterministicForSameSeed) {
  Philox4x32 first(12345, 1);
  Philox4x32 second(12345, 1);
  for (int i = 0; i < 100; ++i) {
    EXPECT_DOUBLE_EQ(first.next_uniform(), second.next_uniform());
  }
}

TEST(Rng, StreamsAreIndependent) {
  Philox4x32 first(999, 1);
  Philox4x32 second(999, 2);
  int differences = 0;
  for (int i = 0; i < 100; ++i) {
    if (first.next_uniform() != second.next_uniform()) {
      ++differences;
    }
  }
  EXPECT_GT(differences, 95);
}

TEST(Rng, UniformIsInUnitInterval) {
  Philox4x32 generator(7, 3);
  for (int i = 0; i < 1000; ++i) {
    const double value = generator.next_uniform();
    EXPECT_GE(value, 0.0);
    EXPECT_LT(value, 1.0);
  }
}

TEST(Rng, NormalHasReasonableMoments) {
  constexpr std::size_t count = 100000;
  std::vector<double> values(count, 0.0);
  Philox4x32::fill_normal(42, 1, 0, values);
  double mean = 0.0;
  for (const double value : values) {
    mean += value;
  }
  mean /= static_cast<double>(count);
  double variance = 0.0;
  for (const double value : values) {
    variance += (value - mean) * (value - mean);
  }
  variance /= static_cast<double>(count);
  EXPECT_NEAR(mean, 0.0, 0.02);
  EXPECT_NEAR(variance, 1.0, 0.05);
}

TEST(Rng, BlockCountersMapToFixedOutputOffsets) {
  constexpr std::size_t values_per_block = 4;
  std::vector<double> whole(1000, 0.0);
  Philox4x32::fill_uniform(11, 5, 0, whole);
  std::vector<double> chunked(1000, 0.0);
  for (std::size_t block = 0; block < 1000 / values_per_block; ++block) {
    const std::size_t offset = block * values_per_block;
    Philox4x32::fill_uniform(11, 5, block,
                             std::span<double>(chunked).subspan(offset, values_per_block));
  }
  for (std::size_t i = 0; i < 1000; ++i) {
    EXPECT_DOUBLE_EQ(whole[i], chunked[i]) << "index=" << i;
  }
}
