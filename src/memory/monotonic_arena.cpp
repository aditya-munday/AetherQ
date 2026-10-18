#include "aetherq/memory/monotonic_arena.hpp"

#include <utility>

#include "aetherq/measurement/clock.hpp"
#include "aetherq/memory/aligned_alloc.hpp"

namespace aetherq {

MonotonicArena::MonotonicArena(std::size_t capacity_bytes, AllocationRecorder* recorder)
    : base_(nullptr), capacity_(capacity_bytes), offset_(0), peak_(0), recorder_(recorder) {
  const std::size_t effective = capacity_bytes == 0 ? simd_alignment : capacity_bytes;
  base_ = static_cast<std::byte*>(aligned_allocate(effective, simd_alignment));
  capacity_ = effective;
}

MonotonicArena::MonotonicArena(MonotonicArena&& other) noexcept
    : base_(std::exchange(other.base_, nullptr)),
      capacity_(std::exchange(other.capacity_, 0)),
      offset_(std::exchange(other.offset_, 0)),
      peak_(std::exchange(other.peak_, 0)),
      recorder_(std::exchange(other.recorder_, nullptr)) {}

MonotonicArena& MonotonicArena::operator=(MonotonicArena&& other) noexcept {
  if (this != &other) {
    if (base_ != nullptr) {
      aligned_release(base_, simd_alignment);
    }
    base_ = std::exchange(other.base_, nullptr);
    capacity_ = std::exchange(other.capacity_, 0);
    offset_ = std::exchange(other.offset_, 0);
    peak_ = std::exchange(other.peak_, 0);
    recorder_ = std::exchange(other.recorder_, nullptr);
  }
  return *this;
}

MonotonicArena::~MonotonicArena() {
  if (base_ != nullptr) {
    aligned_release(base_, simd_alignment);
  }
}

void* MonotonicArena::allocate(std::size_t bytes, std::size_t alignment) {
  const std::uint64_t start = SteadyClock::now_ns();
  const std::size_t aligned_offset = round_up(offset_, alignment);
  if (aligned_offset + bytes > capacity_) {
    if (recorder_ != nullptr) {
      recorder_->record_failure();
    }
    return nullptr;
  }
  void* pointer = base_ + aligned_offset;
  offset_ = aligned_offset + bytes;
  if (offset_ > peak_) {
    peak_ = offset_;
  }
  if (recorder_ != nullptr) {
    recorder_->record_allocation(bytes, SteadyClock::now_ns() - start);
  }
  return pointer;
}

void MonotonicArena::reset() noexcept {
  offset_ = 0;
}

std::size_t MonotonicArena::capacity() const noexcept {
  return capacity_;
}
std::size_t MonotonicArena::used() const noexcept {
  return offset_;
}
std::size_t MonotonicArena::peak_used() const noexcept {
  return peak_;
}

bool MonotonicArena::owns(const void* pointer) const noexcept {
  const auto* bytes = static_cast<const std::byte*>(pointer);
  return bytes >= base_ && bytes < base_ + capacity_;
}

std::span<std::byte> MonotonicArena::storage() noexcept {
  return {base_, capacity_};
}

}
