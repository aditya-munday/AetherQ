#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "aetherq/memory/allocation_metrics.hpp"

namespace aetherq {

class FixedPool {
public:
  FixedPool(std::size_t object_bytes, std::size_t object_count, std::size_t alignment = 64,
            AllocationRecorder* recorder = nullptr);

  FixedPool(FixedPool&&) noexcept;
  FixedPool& operator=(FixedPool&&) noexcept;
  FixedPool(const FixedPool&) = delete;
  FixedPool& operator=(const FixedPool&) = delete;
  ~FixedPool();

  [[nodiscard]] void* allocate();
  void release(void* pointer) noexcept;

  [[nodiscard]] std::size_t object_bytes() const noexcept;
  [[nodiscard]] std::size_t object_count() const noexcept;
  [[nodiscard]] std::size_t available() const noexcept;
  [[nodiscard]] std::size_t in_use() const noexcept;
  [[nodiscard]] bool owns(const void* pointer) const noexcept;

private:
  std::byte* storage_;
  std::vector<std::uint32_t> free_list_;
  std::vector<bool> slot_used_;
  std::size_t object_bytes_;
  std::size_t object_count_;
  std::size_t stride_;
  std::size_t alignment_;
  std::size_t free_count_;
  std::size_t reuse_count_;
  AllocationRecorder* recorder_;
};

}
