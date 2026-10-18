#include "aetherq/memory/pmr_resources.hpp"

#include <cstdlib>

#include "aetherq/measurement/clock.hpp"
#include "aetherq/memory/aligned_alloc.hpp"

namespace aetherq {

SystemMemoryResource::SystemMemoryResource(AllocationRecorder* recorder) : recorder_(recorder) {}

void* SystemMemoryResource::do_allocate(std::size_t bytes, std::size_t alignment) {
  const std::uint64_t start = SteadyClock::now_ns();
  void* pointer = aligned_allocate(
      bytes, alignment < alignof(std::max_align_t) ? alignof(std::max_align_t) : alignment);
  if (recorder_ != nullptr) {
    recorder_->record_allocation(bytes, SteadyClock::now_ns() - start);
  }
  return pointer;
}

void SystemMemoryResource::do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) {
  static_cast<void>(alignment);
  if (recorder_ != nullptr) {
    recorder_->record_deallocation(bytes);
  }
  aligned_release(pointer, 0);
}

bool SystemMemoryResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
  return this == &other;
}

ArenaMemoryResource::ArenaMemoryResource(MonotonicArena* arena) : arena_(arena) {}

void ArenaMemoryResource::reset() {
  arena_->reset();
}

void* ArenaMemoryResource::do_allocate(std::size_t bytes, std::size_t alignment) {
  return arena_->allocate(bytes, alignment);
}

void ArenaMemoryResource::do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) {
  static_cast<void>(pointer);
  static_cast<void>(bytes);
  static_cast<void>(alignment);
}

bool ArenaMemoryResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
  return this == &other;
}

PoolMemoryResource::PoolMemoryResource(FixedPool* pool) : pool_(pool) {}

void* PoolMemoryResource::do_allocate(std::size_t bytes, std::size_t alignment) {
  static_cast<void>(alignment);
  if (bytes > pool_->object_bytes()) {
    return nullptr;
  }
  return pool_->allocate();
}

void PoolMemoryResource::do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) {
  static_cast<void>(bytes);
  static_cast<void>(alignment);
  pool_->release(pointer);
}

bool PoolMemoryResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
  return this == &other;
}

}
