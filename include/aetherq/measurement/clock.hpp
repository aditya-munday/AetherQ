#pragma once

#include <cstdint>

namespace aetherq {

class SteadyClock {
public:
  [[nodiscard]] static std::uint64_t now_ns() noexcept;
  [[nodiscard]] static double now_seconds() noexcept;
};

}
