#include <cmath>
#include <cstddef>

#include "aetherq/kernels/dispatch.hpp"
#include "aetherq/kernels/kernels.hpp"

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define AETHERQ_HAS_X86_INTRINSICS 1
#else
#define AETHERQ_HAS_X86_INTRINSICS 0
#endif

namespace aetherq {

namespace {

constexpr std::size_t tile_rows = 64;
constexpr std::size_t tile_inner = 64;
constexpr std::size_t tile_cols = 64;

double pairwise_sum(const double* values, std::size_t count) {
  if (count <= 8) {
    double total = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
      total += values[i];
    }
    return total;
  }
  const std::size_t half = count / 2;
  return pairwise_sum(values, half) + pairwise_sum(values + half, count - half);
}

bool avx2_available() {
  static const bool available = detect_cpu_features().avx2;
  return available;
}

#if AETHERQ_HAS_X86_INTRINSICS
__attribute__((target("avx2,fma"))) double avx2_dot_impl(const double* lhs, const double* rhs,
                                                         std::size_t count) {
  __m256d acc0 = _mm256_setzero_pd();
  __m256d acc1 = _mm256_setzero_pd();
  std::size_t i = 0;
  for (; i + 8 <= count; i += 8) {
    const __m256d a0 = _mm256_loadu_pd(lhs + i);
    const __m256d b0 = _mm256_loadu_pd(rhs + i);
    const __m256d a1 = _mm256_loadu_pd(lhs + i + 4);
    const __m256d b1 = _mm256_loadu_pd(rhs + i + 4);
    acc0 = _mm256_fmadd_pd(a0, b0, acc0);
    acc1 = _mm256_fmadd_pd(a1, b1, acc1);
  }
  const __m256d sum = _mm256_add_pd(acc0, acc1);
  const __m128d low = _mm256_castpd256_pd128(sum);
  const __m128d high = _mm256_extractf128_pd(sum, 1);
  const __m128d pair = _mm_add_pd(low, high);
  const __m128d swapped = _mm_shuffle_pd(pair, pair, 1);
  const __m128d final = _mm_add_sd(pair, swapped);
  double total = _mm_cvtsd_f64(final);
  for (; i < count; ++i) {
    total += lhs[i] * rhs[i];
  }
  return total;
}

__attribute__((target("avx2,fma"))) double avx2_sum_impl(const double* values, std::size_t count) {
  __m256d acc0 = _mm256_setzero_pd();
  __m256d acc1 = _mm256_setzero_pd();
  std::size_t i = 0;
  for (; i + 8 <= count; i += 8) {
    acc0 = _mm256_add_pd(acc0, _mm256_loadu_pd(values + i));
    acc1 = _mm256_add_pd(acc1, _mm256_loadu_pd(values + i + 4));
  }
  const __m256d sum = _mm256_add_pd(acc0, acc1);
  const __m128d low = _mm256_castpd256_pd128(sum);
  const __m128d high = _mm256_extractf128_pd(sum, 1);
  const __m128d pair = _mm_add_pd(low, high);
  const __m128d swapped = _mm_shuffle_pd(pair, pair, 1);
  const __m128d final = _mm_add_sd(pair, swapped);
  double total = _mm_cvtsd_f64(final);
  for (; i < count; ++i) {
    total += values[i];
  }
  return total;
}

__attribute__((target("avx2,fma"))) void avx2_scale_impl(double factor, const double* input,
                                                         double* output, std::size_t count) {
  const __m256d scale = _mm256_set1_pd(factor);
  std::size_t i = 0;
  for (; i + 4 <= count; i += 4) {
    const __m256d value = _mm256_loadu_pd(input + i);
    _mm256_storeu_pd(output + i, _mm256_mul_pd(scale, value));
  }
  for (; i < count; ++i) {
    output[i] = factor * input[i];
  }
}

__attribute__((target("avx2,fma"))) void avx2_axpy_impl(double factor, const double* x, double* y,
                                                        std::size_t count) {
  const __m256d scale = _mm256_set1_pd(factor);
  std::size_t i = 0;
  for (; i + 4 <= count; i += 4) {
    const __m256d xv = _mm256_loadu_pd(x + i);
    const __m256d yv = _mm256_loadu_pd(y + i);
    _mm256_storeu_pd(y + i, _mm256_fmadd_pd(scale, xv, yv));
  }
  for (; i < count; ++i) {
    y[i] += factor * x[i];
  }
}
#endif

}

double optimized_dot(std::span<const double> lhs, std::span<const double> rhs) {
  const std::size_t count = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
#if AETHERQ_HAS_X86_INTRINSICS
  if (avx2_available()) {
    return avx2_dot_impl(lhs.data(), rhs.data(), count);
  }
#endif
  double partials[4] = {0.0, 0.0, 0.0, 0.0};
  const std::size_t blocked = count - (count % 4);
  for (std::size_t i = 0; i < blocked; i += 4) {
    partials[0] += lhs[i] * rhs[i];
    partials[1] += lhs[i + 1] * rhs[i + 1];
    partials[2] += lhs[i + 2] * rhs[i + 2];
    partials[3] += lhs[i + 3] * rhs[i + 3];
  }
  double total = (partials[0] + partials[1]) + (partials[2] + partials[3]);
  for (std::size_t i = blocked; i < count; ++i) {
    total += lhs[i] * rhs[i];
  }
  return total;
}

double optimized_sum(std::span<const double> values) {
#if AETHERQ_HAS_X86_INTRINSICS
  if (avx2_available()) {
    return avx2_sum_impl(values.data(), values.size());
  }
#endif
  return pairwise_sum(values.data(), values.size());
}

double optimized_norm(std::span<const double> values) {
  return std::sqrt(optimized_dot(values, values));
}

void optimized_scale(double factor, std::span<const double> input, std::span<double> output) {
  const std::size_t count = input.size() < output.size() ? input.size() : output.size();
#if AETHERQ_HAS_X86_INTRINSICS
  if (avx2_available()) {
    avx2_scale_impl(factor, input.data(), output.data(), count);
    return;
  }
#endif
  for (std::size_t i = 0; i < count; ++i) {
    output[i] = factor * input[i];
  }
}

void optimized_axpy(double factor, std::span<const double> x, std::span<double> y) {
  const std::size_t count = x.size() < y.size() ? x.size() : y.size();
#if AETHERQ_HAS_X86_INTRINSICS
  if (avx2_available()) {
    avx2_axpy_impl(factor, x.data(), y.data(), count);
    return;
  }
#endif
  for (std::size_t i = 0; i < count; ++i) {
    y[i] += factor * x[i];
  }
}

void optimized_matrix_vector(std::span<const double> matrix, std::size_t rows, std::size_t cols,
                             std::span<const double> vector, std::span<double> output) {
  for (std::size_t row = 0; row < rows; ++row) {
    const std::size_t base = row * cols;
    output[row] = optimized_dot(matrix.subspan(base, cols), vector);
  }
}

void optimized_matrix_matrix(std::span<const double> lhs, std::size_t lhs_rows, std::size_t shared,
                             std::span<const double> rhs, std::size_t rhs_cols,
                             std::span<double> output) {
  for (std::size_t row = 0; row < lhs_rows; ++row) {
    const std::size_t out_base = row * rhs_cols;
    for (std::size_t col = 0; col < rhs_cols; ++col) {
      output[out_base + col] = 0.0;
    }
  }
  for (std::size_t row_tile = 0; row_tile < lhs_rows; row_tile += tile_rows) {
    const std::size_t row_end = row_tile + tile_rows < lhs_rows ? row_tile + tile_rows : lhs_rows;
    for (std::size_t inner_tile = 0; inner_tile < shared; inner_tile += tile_inner) {
      const std::size_t inner_end =
          inner_tile + tile_inner < shared ? inner_tile + tile_inner : shared;
      for (std::size_t col_tile = 0; col_tile < rhs_cols; col_tile += tile_cols) {
        const std::size_t col_end =
            col_tile + tile_cols < rhs_cols ? col_tile + tile_cols : rhs_cols;
        for (std::size_t row = row_tile; row < row_end; ++row) {
          const std::size_t lhs_base = row * shared;
          const std::size_t out_base = row * rhs_cols;
          for (std::size_t inner = inner_tile; inner < inner_end; ++inner) {
            const double scale = lhs[lhs_base + inner];
            const std::size_t rhs_base = inner * rhs_cols;
            for (std::size_t col = col_tile; col < col_end; ++col) {
              output[out_base + col] += scale * rhs[rhs_base + col];
            }
          }
        }
      }
    }
  }
}

void optimized_covariance(std::span<const double> returns, std::size_t observations,
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

void optimized_weighted_sum(std::span<const double> values, std::span<const double> weights,
                            std::span<double> output) {
  const std::size_t assets = weights.size();
  const std::size_t observations = values.size() / assets;
  for (std::size_t row = 0; row < observations; ++row) {
    output[row] = optimized_dot(values.subspan(row * assets, assets), weights);
  }
}

void optimized_exp(std::span<const double> input, std::span<double> output) {
  const std::size_t count = input.size() < output.size() ? input.size() : output.size();
  for (std::size_t i = 0; i < count; ++i) {
    output[i] = std::exp(input[i]);
  }
}

void optimized_log(std::span<const double> input, std::span<double> output) {
  const std::size_t count = input.size() < output.size() ? input.size() : output.size();
  for (std::size_t i = 0; i < count; ++i) {
    output[i] = std::log(input[i]);
  }
}

void optimized_normal_cdf(std::span<const double> input, std::span<double> output) {
  const std::size_t count = input.size() < output.size() ? input.size() : output.size();
  constexpr double inv_sqrt_two = 0.70710678118654752440;
  for (std::size_t i = 0; i < count; ++i) {
    output[i] = 0.5 * std::erfc(-input[i] * inv_sqrt_two);
  }
}

}
