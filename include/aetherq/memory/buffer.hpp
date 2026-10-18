#pragma once

#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

#include "aetherq/memory/aligned_alloc.hpp"

namespace aetherq {

template<typename T>
class Buffer {
public:
  Buffer() = default;

  explicit Buffer(std::size_t count) : count_(count), data_(allocate_objects<T>(count)) {}

  Buffer(Buffer&& other) noexcept : count_(other.count_), data_(other.data_), arena_(other.arena_) {
    other.count_ = 0;
    other.data_ = nullptr;
    other.arena_ = nullptr;
  }

  Buffer& operator=(Buffer&& other) noexcept {
    if (this != &other) {
      release();
      count_ = other.count_;
      data_ = other.data_;
      arena_ = other.arena_;
      other.count_ = 0;
      other.data_ = nullptr;
      other.arena_ = nullptr;
    }
    return *this;
  }

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  ~Buffer() {
    release();
  }

  static Buffer view(T* data, std::size_t count) {
    Buffer buffer;
    buffer.data_ = data;
    buffer.count_ = count;
    buffer.arena_ = data;
    return buffer;
  }

  [[nodiscard]] T* data() noexcept {
    return data_;
  }
  [[nodiscard]] const T* data() const noexcept {
    return data_;
  }
  [[nodiscard]] std::size_t size() const noexcept {
    return count_;
  }
  [[nodiscard]] bool empty() const noexcept {
    return count_ == 0;
  }

  [[nodiscard]] T& operator[](std::size_t index) noexcept {
    return data_[index];
  }
  [[nodiscard]] const T& operator[](std::size_t index) const noexcept {
    return data_[index];
  }

  [[nodiscard]] std::span<T> span() noexcept {
    return {data_, count_};
  }
  [[nodiscard]] std::span<const T> span() const noexcept {
    return {data_, count_};
  }

  [[nodiscard]] T* begin() noexcept {
    return data_;
  }
  [[nodiscard]] T* end() noexcept {
    return data_ + count_;
  }
  [[nodiscard]] const T* begin() const noexcept {
    return data_;
  }
  [[nodiscard]] const T* end() const noexcept {
    return data_ + count_;
  }

  void fill(const T& value) {
    for (std::size_t i = 0; i < count_; ++i) {
      data_[i] = value;
    }
  }

private:
  void release() noexcept {
    if (data_ != nullptr && arena_ == nullptr) {
      release_objects(data_, count_);
    }
    data_ = nullptr;
    count_ = 0;
    arena_ = nullptr;
  }

  std::size_t count_{0};
  T* data_{nullptr};
  T* arena_{nullptr};
};

}
