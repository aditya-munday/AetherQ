#include <cmath>
#include <cstddef>

#include "aetherq/kernels/kernels.hpp"

namespace aetherq {

double scalar_dot(std::span<const double> lhs, std::span<const double> rhs) {
  const std::size_t count = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
  double total = 0.0;
  for (std::size_t i = 0; i < count; ++i) {
    total += lhs[i] * rhs[i];
  }
  return total;
}

double scalar_sum(std::span<const double> values) {
  const std::size_t count = values.size();
  if (count == 0) {
    return 0.0;
  }
  double partials[4] = {0.0, 0.0, 0.0, 0.0};
  const std::size_t blocked = count - (count % 4);
  for (std::size_t i = 0; i < blocked; i += 4) {
    partials[0] += values[i];
    partials[1] += values[i + 1];
    partials[2] += values[i + 2];
    partials[3] += values[i + 3];
  }
  double total = (partials[0] + partials[1]) + (partials[2] + partials[3]);
  for (std::size_t i = blocked; i < count; ++i) {
    total += values[i];
  }
  return total;
}

double scalar_norm(std::span<const double> values) {
  return std::sqrt(scalar_dot(values, values));
}

void scalar_scale(double factor, std::span<const double> input, std::span<double> output) {
  const std::size_t count = input.size() < output.size() ? input.size() : output.size();
  for (std::size_t i = 0; i < count; ++i) {
    output[i] = factor * input[i];
  }
}

void scalar_axpy(double factor, std::span<const double> x, std::span<double> y) {
  const std::size_t count = x.size() < y.size() ? x.size() : y.size();
  for (std::size_t i = 0; i < count; ++i) {
    y[i] += factor * x[i];
  }
}

void scalar_matrix_vector(std::span<const double> matrix, std::size_t rows, std::size_t cols,
                          std::span<const double> vector, std::span<double> output) {
  for (std::size_t row = 0; row < rows; ++row) {
    double total = 0.0;
    const std::size_t base = row * cols;
    for (std::size_t col = 0; col < cols; ++col) {
      total += matrix[base + col] * vector[col];
    }
    output[row] = total;
  }
}

void scalar_matrix_matrix(std::span<const double> lhs, std::size_t lhs_rows, std::size_t shared,
                          std::span<const double> rhs, std::size_t rhs_cols,
                          std::span<double> output) {
  for (std::size_t row = 0; row < lhs_rows; ++row) {
    const std::size_t lhs_base = row * shared;
    const std::size_t out_base = row * rhs_cols;
    for (std::size_t col = 0; col < rhs_cols; ++col) {
      output[out_base + col] = 0.0;
    }
    for (std::size_t inner = 0; inner < shared; ++inner) {
      const double scale = lhs[lhs_base + inner];
      const std::size_t rhs_base = inner * rhs_cols;
      for (std::size_t col = 0; col < rhs_cols; ++col) {
        output[out_base + col] += scale * rhs[rhs_base + col];
      }
    }
  }
}

void scalar_covariance(std::span<const double> returns, std::size_t observations,
                       std::size_t assets, std::span<double> covariance) {
  for (std::size_t asset = 0; asset < assets; ++asset) {
    double mean = 0.0;
    for (std::size_t row = 0; row < observations; ++row) {
      mean += returns[row * assets + asset];
    }
    mean /= static_cast<double>(observations);
    for (std::size_t other = asset; other < assets; ++other) {
      double other_mean = 0.0;
      for (std::size_t row = 0; row < observations; ++row) {
        other_mean += returns[row * assets + other];
      }
      other_mean /= static_cast<double>(observations);
      double total = 0.0;
      for (std::size_t row = 0; row < observations; ++row) {
        total +=
            (returns[row * assets + asset] - mean) * (returns[row * assets + other] - other_mean);
      }
      const double value = total / static_cast<double>(observations);
      covariance[asset * assets + other] = value;
      covariance[other * assets + asset] = value;
    }
  }
}

void scalar_weighted_sum(std::span<const double> values, std::span<const double> weights,
                         std::span<double> output) {
  const std::size_t assets = weights.size();
  const std::size_t observations = values.size() / assets;
  for (std::size_t row = 0; row < observations; ++row) {
    double total = 0.0;
    const std::size_t base = row * assets;
    for (std::size_t asset = 0; asset < assets; ++asset) {
      total += values[base + asset] * weights[asset];
    }
    output[row] = total;
  }
}

void scalar_exp(std::span<const double> input, std::span<double> output) {
  const std::size_t count = input.size() < output.size() ? input.size() : output.size();
  for (std::size_t i = 0; i < count; ++i) {
    output[i] = std::exp(input[i]);
  }
}

void scalar_log(std::span<const double> input, std::span<double> output) {
  const std::size_t count = input.size() < output.size() ? input.size() : output.size();
  for (std::size_t i = 0; i < count; ++i) {
    output[i] = std::log(input[i]);
  }
}

void scalar_normal_cdf(std::span<const double> input, std::span<double> output) {
  const std::size_t count = input.size() < output.size() ? input.size() : output.size();
  constexpr double inv_sqrt_two = 0.70710678118654752440;
  for (std::size_t i = 0; i < count; ++i) {
    output[i] = 0.5 * std::erfc(-input[i] * inv_sqrt_two);
  }
}

}
