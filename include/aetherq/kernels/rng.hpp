#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace aetherq {

class Philox4x32 {
public:
  explicit Philox4x32(std::uint64_t seed, std::uint64_t stream = 0) noexcept;

  std::uint32_t next_u32() noexcept;
  double next_uniform() noexcept;
  double next_normal() noexcept;

  static void fill_uniform(std::uint64_t seed, std::uint64_t stream, std::uint64_t counter_base,
                           std::span<double> output) noexcept;
  static void fill_normal(std::uint64_t seed, std::uint64_t stream, std::uint64_t counter_base,
                          std::span<double> output) noexcept;

private:
  std::uint64_t counter_{0};
  std::uint64_t stream_;
  std::uint32_t key0_;
  std::uint32_t key1_;
  std::uint32_t buffer_[4]{0, 0, 0, 0};
  std::size_t buffer_index_{4};
  bool has_spare_normal_{false};
  double spare_normal_{0.0};
};

}
