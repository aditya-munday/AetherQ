#include "aetherq/kernels/rng.hpp"

#include <cmath>
#include <cstddef>

namespace aetherq {

namespace {

constexpr std::uint32_t philox_m0 = 0xD2511F53u;
constexpr std::uint32_t philox_m1 = 0xCD9E8D57u;
constexpr std::uint32_t philox_w0 = 0x9E3779B9u;
constexpr std::uint32_t philox_w1 = 0xBB67AE85u;

void philox_round(std::uint32_t& c0, std::uint32_t& c1, std::uint32_t& c2, std::uint32_t& c3,
                  std::uint32_t k0, std::uint32_t k1) {
  const std::uint64_t product0 =
      static_cast<std::uint64_t>(philox_m0) * static_cast<std::uint64_t>(c0);
  const std::uint64_t product1 =
      static_cast<std::uint64_t>(philox_m1) * static_cast<std::uint64_t>(c2);
  const std::uint32_t high0 = static_cast<std::uint32_t>(product0 >> 32);
  const std::uint32_t low0 = static_cast<std::uint32_t>(product0);
  const std::uint32_t high1 = static_cast<std::uint32_t>(product1 >> 32);
  const std::uint32_t low1 = static_cast<std::uint32_t>(product1);
  c0 = high1 ^ c1 ^ k0;
  c1 = low1;
  c2 = high0 ^ c3 ^ k1;
  c3 = low0;
}

void philox_block(std::uint32_t& c0, std::uint32_t& c1, std::uint32_t& c2, std::uint32_t& c3,
                  std::uint32_t k0, std::uint32_t k1) {
  for (int round = 0; round < 10; ++round) {
    philox_round(c0, c1, c2, c3, k0, k1);
    k0 += philox_w0;
    k1 += philox_w1;
  }
}

double uint32_to_unit(std::uint32_t value) noexcept {
  constexpr double scale = 2.3283064365386963e-10;
  return (static_cast<double>(value) + 0.5) * scale;
}

}

Philox4x32::Philox4x32(std::uint64_t seed, std::uint64_t stream) noexcept
    : counter_(0),
      stream_(stream),
      key0_(static_cast<std::uint32_t>(seed)),
      key1_(static_cast<std::uint32_t>(seed >> 32)),
      buffer_{0, 0, 0, 0},
      buffer_index_(4),
      has_spare_normal_(false),
      spare_normal_(0.0) {}

std::uint32_t Philox4x32::next_u32() noexcept {
  if (buffer_index_ >= 4) {
    std::uint32_t c0 = static_cast<std::uint32_t>(counter_);
    std::uint32_t c1 = static_cast<std::uint32_t>(counter_ >> 32);
    std::uint32_t c2 = static_cast<std::uint32_t>(stream_);
    std::uint32_t c3 = static_cast<std::uint32_t>(stream_ >> 32);
    std::uint32_t k0 = key0_;
    std::uint32_t k1 = key1_;
    philox_block(c0, c1, c2, c3, k0, k1);
    buffer_[0] = c0;
    buffer_[1] = c1;
    buffer_[2] = c2;
    buffer_[3] = c3;
    buffer_index_ = 0;
    counter_ += 1;
  }
  return buffer_[buffer_index_++];
}

double Philox4x32::next_uniform() noexcept {
  return uint32_to_unit(next_u32());
}

double Philox4x32::next_normal() noexcept {
  if (has_spare_normal_) {
    has_spare_normal_ = false;
    return spare_normal_;
  }
  double u1 = next_uniform();
  if (u1 <= 0.0) {
    u1 = 1e-12;
  }
  const double u2 = next_uniform();
  const double radius = std::sqrt(-2.0 * std::log(u1));
  const double angle = 6.28318530717958647692 * u2;
  spare_normal_ = radius * std::sin(angle);
  has_spare_normal_ = true;
  return radius * std::cos(angle);
}

void Philox4x32::fill_uniform(std::uint64_t seed, std::uint64_t stream, std::uint64_t counter_base,
                              std::span<double> output) noexcept {
  Philox4x32 generator(seed, stream);
  generator.counter_ = counter_base;
  for (std::size_t i = 0; i < output.size(); ++i) {
    output[i] = generator.next_uniform();
  }
}

void Philox4x32::fill_normal(std::uint64_t seed, std::uint64_t stream, std::uint64_t counter_base,
                             std::span<double> output) noexcept {
  Philox4x32 generator(seed, stream);
  generator.counter_ = counter_base;
  for (std::size_t i = 0; i < output.size(); ++i) {
    output[i] = generator.next_normal();
  }
}

}
