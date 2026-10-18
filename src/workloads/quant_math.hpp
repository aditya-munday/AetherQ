#pragma once

#include <cmath>

namespace aetherq {

inline constexpr double inverse_sqrt_two = 0.70710678118654752440;
inline constexpr double inverse_sqrt_two_pi = 0.39894228040143267794;
inline constexpr double two_pi = 6.28318530717958647692;

inline double standard_normal_cdf(double x) noexcept {
  return 0.5 * std::erfc(-x * inverse_sqrt_two);
}

inline double standard_normal_pdf(double x) noexcept {
  return inverse_sqrt_two_pi * std::exp(-0.5 * x * x);
}

}
