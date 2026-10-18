#pragma once

#include <cstddef>
#include <memory_resource>

#include "aetherq/memory/allocation_metrics.hpp"
#include "aetherq/memory/fixed_pool.hpp"
#include "aetherq/memory/monotonic_arena.hpp"

namespace aetherq {

class SystemMemoryResource final : public std::pmr::memory_resource {
public:
  explicit SystemMemoryResource(AllocationRecorder* recorder = nullptr);

private:
  void* do_allocate(std::size_t bytes, std::size_t alignment) override;
  void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override;
  [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

  AllocationRecorder* recorder_;
};

class ArenaMemoryResource final : public std::pmr::memory_resource {
public:
  explicit ArenaMemoryResource(MonotonicArena* arena);

  void reset();

private:
  void* do_allocate(std::size_t bytes, std::size_t alignment) override;
  void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override;
  [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

  MonotonicArena* arena_;
};

class PoolMemoryResource final : public std::pmr::memory_resource {
public:
  explicit PoolMemoryResource(FixedPool* pool);

private:
  void* do_allocate(std::size_t bytes, std::size_t alignment) override;
  void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override;
  [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

  FixedPool* pool_;
};

}
