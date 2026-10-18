#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include "aetherq/memory/allocation_metrics.hpp"

namespace aetherq {

class MonotonicArena {
public:
  explicit MonotonicArena(std::size_t capacity_bytes, AllocationRecorder* recorder = nullptr);

  MonotonicArena(MonotonicArena&&) noexcept;
  MonotonicArena& operator=(MonotonicArena&&) noexcept;
  MonotonicArena(const MonotonicArena&) = delete;
  MonotonicArena& operator=(const MonotonicArena&) = delete;
  ~MonotonicArena();

  [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment);
  void reset() noexcept;

  [[nodiscard]] std::size_t capacity() const noexcept;
  [[nodiscard]] std::size_t used() const noexcept;
  [[nodiscard]] std::size_t peak_used() const noexcept;
  [[nodiscard]] bool owns(const void* pointer) const noexcept;
  [[nodiscard]] std::span<std::byte> storage() noexcept;

private:
  std::byte* base_;
  std::size_t capacity_;
  std::size_t offset_;
  std::size_t peak_;
  AllocationRecorder* recorder_;
};

}
