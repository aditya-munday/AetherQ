#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "aetherq/kernels/kernels.hpp"

using namespace aetherq;

namespace {

std::vector<double> geometric(std::size_t count, double ratio) {
  std::vector<double> values(count);
  double value = 1.0;
  for (std::size_t i = 0; i < count; ++i) {
    values[i] = value;
    value *= ratio;
  }
  return values;
}

}

TEST(NumericalKernels, DotEmptyReturnsZero) {
  EXPECT_DOUBLE_EQ(scalar_dot({}, {}), 0.0);
  EXPECT_DOUBLE_EQ(optimized_dot({}, {}), 0.0);
}

TEST(NumericalKernels, DotSingleElement) {
  const std::vector<double> lhs{3.0};
  const std::vector<double> rhs{4.0};
  EXPECT_DOUBLE_EQ(scalar_dot(lhs, rhs), 12.0);
  EXPECT_DOUBLE_EQ(optimized_dot(lhs, rhs), 12.0);
}

TEST(NumericalKernels, DotHandlesMismatchedLengths) {
  const std::vector<double> lhs{1.0, 2.0, 3.0};
  const std::vector<double> rhs{1.0, 1.0};
  EXPECT_DOUBLE_EQ(scalar_dot(lhs, rhs), 3.0);
  EXPECT_DOUBLE_EQ(optimized_dot(lhs, rhs), 3.0);
}

TEST(NumericalKernels, SumLargeMagnitudesRelativeError) {
  const auto values = geometric(10000, 1.0001);
  const double scalar = scalar_sum(values);
  const double optimized = optimized_sum(values);
  EXPECT_NEAR(scalar, optimized, std::fabs(scalar) * 1e-12);
}

TEST(NumericalKernels, SumDenormalsAreHandled) {
  std::vector<double> values(64, std::numeric_limits<double>::denorm_min());
  const double scalar = scalar_sum(values);
  const double optimized = optimized_sum(values);
  EXPECT_DOUBLE_EQ(scalar, optimized);
}

TEST(NumericalKernels, SumUnaryNegativesCancel) {
  std::vector<double> values;
  for (int i = 0; i < 500; ++i) {
    values.push_back(1.0);
    values.push_back(-1.0);
  }
  const double scalar = scalar_sum(values);
  const double optimized = optimized_sum(values);
  EXPECT_NEAR(scalar, 0.0, 1e-9);
  EXPECT_NEAR(optimized, 0.0, 1e-9);
}

TEST(NumericalKernels, MatrixVectorUnitializedRowsAreWritten) {
  const std::size_t rows = 5;
  const std::size_t cols = 4;
  std::vector<double> matrix(rows * cols, 1.0);
  std::vector<double> vector(cols, 2.0);
  std::vector<double> scalar_output(rows, -1.0);
  std::vector<double> optimized_output(rows, -1.0);
  scalar_matrix_vector(matrix, rows, cols, vector, scalar_output);
  optimized_matrix_vector(matrix, rows, cols, vector, optimized_output);
  for (std::size_t i = 0; i < rows; ++i) {
    EXPECT_DOUBLE_EQ(scalar_output[i], 8.0);
    EXPECT_DOUBLE_EQ(optimized_output[i], 8.0);
  }
}

TEST(NumericalKernels, CovarianceIsSymmetric) {
  const std::size_t observations = 100;
  const std::size_t assets = 6;
  std::vector<double> returns(observations * assets);
  for (std::size_t i = 0; i < returns.size(); ++i) {
    returns[i] = std::cos(static_cast<double>(i) * 0.07);
  }
  std::vector<double> covariance(assets * assets, 0.0);
  scalar_covariance(returns, observations, assets, covariance);
  for (std::size_t i = 0; i < assets; ++i) {
    for (std::size_t j = 0; j < assets; ++j) {
      EXPECT_DOUBLE_EQ(covariance[i * assets + j], covariance[j * assets + i]);
    }
  }
  for (std::size_t i = 0; i < assets; ++i) {
    EXPECT_GE(covariance[i * assets + i], 0.0);
  }
}

TEST(NumericalKernels, OptimizedExpMatchesScalarWithinUlps) {
  const std::size_t size = 1000;
  std::vector<double> input(size);
  for (std::size_t i = 0; i < size; ++i) {
    input[i] = -10.0 + (static_cast<double>(i) * 0.02);
  }
  std::vector<double> scalar_output(size, 0.0);
  std::vector<double> optimized_output(size, 0.0);
  scalar_exp(input, scalar_output);
  optimized_exp(input, optimized_output);
  for (std::size_t i = 0; i < size; ++i) {
    const double tolerance =
        std::fabs(scalar_output[i]) * 4.0 * std::numeric_limits<double>::epsilon();
    EXPECT_LE(std::fabs(scalar_output[i] - optimized_output[i]), tolerance + 1e-300) << "i=" << i;
  }
}

TEST(NumericalKernels, NormalCdfMatchesErfcReference) {
  const std::size_t size = 500;
  std::vector<double> input(size);
  for (std::size_t i = 0; i < size; ++i) {
    input[i] = -6.0 + (static_cast<double>(i) * 0.024);
  }
  std::vector<double> output(size, 0.0);
  scalar_normal_cdf(input, output);
  for (std::size_t i = 0; i < size; ++i) {
    const double reference = 0.5 * std::erfc(-input[i] * 0.70710678118654752440);
    EXPECT_NEAR(output[i], reference, 1e-15);
  }
}
