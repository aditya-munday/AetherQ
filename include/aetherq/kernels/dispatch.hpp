#pragma once

#include <cstddef>
#include <string_view>

#include "aetherq/kernels/kernels.hpp"
#include "aetherq/runtime/config.hpp"

namespace aetherq {

struct CpuFeatures {
  bool sse42{false};
  bool avx2{false};
  bool avx512f{false};
  bool neon{false};
};

CpuFeatures detect_cpu_features() noexcept;
std::string_view best_backend_name(const CpuFeatures& features) noexcept;

struct KernelSet {
  double (*dot)(std::span<const double>, std::span<const double>);
  double (*sum)(std::span<const double>);
  double (*norm)(std::span<const double>);
  void (*scale)(double, std::span<const double>, std::span<double>);
  void (*axpy)(double, std::span<const double>, std::span<double>);
  void (*matrix_vector)(std::span<const double>, std::size_t, std::size_t, std::span<const double>,
                        std::span<double>);
  void (*matrix_matrix)(std::span<const double>, std::size_t, std::size_t, std::span<const double>,
                        std::size_t, std::span<double>);
  void (*covariance)(std::span<const double>, std::size_t, std::size_t, std::span<double>);
  void (*weighted_sum)(std::span<const double>, std::span<const double>, std::span<double>);
  void (*exp_vector)(std::span<const double>, std::span<double>);
  void (*log_vector)(std::span<const double>, std::span<double>);
  void (*normal_cdf_vector)(std::span<const double>, std::span<double>);
};

KernelSet select_kernels(Backend backend);

}
