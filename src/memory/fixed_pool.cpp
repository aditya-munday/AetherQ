#include "aetherq/memory/fixed_pool.hpp"

#include <utility>

#include "aetherq/measurement/clock.hpp"
#include "aetherq/memory/aligned_alloc.hpp"

namespace aetherq {

FixedPool::FixedPool(std::size_t object_bytes, std::size_t object_count, std::size_t alignment,
                     AllocationRecorder* recorder)
    : storage_(nullptr),
      object_bytes_(object_bytes),
      object_count_(object_count),
      stride_(round_up(object_bytes, alignment)),
      alignment_(alignment),
      free_count_(object_count),
      reuse_count_(0),
      recorder_(recorder) {
  const std::size_t effective_count = object_count == 0 ? 1 : object_count;
  const std::size_t effective_stride = stride_ == 0 ? alignment : stride_;
  storage_ =
      static_cast<std::byte*>(aligned_allocate(effective_stride * effective_count, alignment));
  free_list_.reserve(effective_count);
  for (std::size_t i = 0; i < effective_count; ++i) {
    free_list_.push_back(static_cast<std::uint32_t>(effective_count - 1 - i));
  }
  slot_used_.assign(effective_count, false);
  object_count_ = effective_count;
  free_count_ = effective_count;
  if (recorder_ != nullptr) {
    recorder_->record_allocation(effective_stride * effective_count, 0);
  }
}

FixedPool::FixedPool(FixedPool&& other) noexcept
    : storage_(std::exchange(other.storage_, nullptr)),
      free_list_(std::move(other.free_list_)),
      slot_used_(std::move(other.slot_used_)),
      object_bytes_(std::exchange(other.object_bytes_, 0)),
      object_count_(std::exchange(other.object_count_, 0)),
      stride_(std::exchange(other.stride_, 0)),
      alignment_(std::exchange(other.alignment_, 0)),
      free_count_(std::exchange(other.free_count_, 0)),
      reuse_count_(std::exchange(other.reuse_count_, 0)),
      recorder_(std::exchange(other.recorder_, nullptr)) {}

FixedPool& FixedPool::operator=(FixedPool&& other) noexcept {
  if (this != &other) {
    if (storage_ != nullptr) {
      aligned_release(storage_, alignment_);
    }
    storage_ = std::exchange(other.storage_, nullptr);
    free_list_ = std::move(other.free_list_);
    slot_used_ = std::move(other.slot_used_);
    object_bytes_ = std::exchange(other.object_bytes_, 0);
    object_count_ = std::exchange(other.object_count_, 0);
    stride_ = std::exchange(other.stride_, 0);
    alignment_ = std::exchange(other.alignment_, 0);
    free_count_ = std::exchange(other.free_count_, 0);
    reuse_count_ = std::exchange(other.reuse_count_, 0);
    recorder_ = std::exchange(other.recorder_, nullptr);
  }
  return *this;
}

FixedPool::~FixedPool() {
  if (storage_ != nullptr) {
    aligned_release(storage_, alignment_);
  }
}

void* FixedPool::allocate() {
  const std::uint64_t start = SteadyClock::now_ns();
  if (free_list_.empty()) {
    if (recorder_ != nullptr) {
      recorder_->record_failure();
    }
    return nullptr;
  }
  const std::uint32_t slot = free_list_.back();
  free_list_.pop_back();
  slot_used_[slot] = true;
  --free_count_;
  ++reuse_count_;
  if (recorder_ != nullptr) {
    recorder_->record_reuse();
    recorder_->record_allocation(object_bytes_, SteadyClock::now_ns() - start);
  }
  return storage_ + (static_cast<std::size_t>(slot) * stride_);
}

void FixedPool::release(void* pointer) noexcept {
  if (pointer == nullptr || !owns(pointer)) {
    return;
  }
  const auto offset = static_cast<std::size_t>(static_cast<std::byte*>(pointer) - storage_);
  const auto slot = static_cast<std::uint32_t>(offset / stride_);
  if (slot >= object_count_ || !slot_used_[slot]) {
    return;
  }
  slot_used_[slot] = false;
  free_list_.push_back(slot);
  ++free_count_;
  if (recorder_ != nullptr) {
    recorder_->record_deallocation(object_bytes_);
  }
}

std::size_t FixedPool::object_bytes() const noexcept {
  return object_bytes_;
}
std::size_t FixedPool::object_count() const noexcept {
  return object_count_;
}
std::size_t FixedPool::available() const noexcept {
  return free_count_;
}
std::size_t FixedPool::in_use() const noexcept {
  return object_count_ - free_count_;
}

bool FixedPool::owns(const void* pointer) const noexcept {
  const auto* bytes = static_cast<const std::byte*>(pointer);
  return bytes >= storage_ && bytes < storage_ + (stride_ * object_count_);
}

}
