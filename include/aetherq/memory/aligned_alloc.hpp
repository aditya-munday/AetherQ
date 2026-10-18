#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <type_traits>

namespace aetherq {

constexpr std::size_t cache_line_bytes = 64;
constexpr std::size_t simd_alignment = 64;

bool is_aligned(const void* pointer, std::size_t alignment) noexcept;
std::size_t round_up(std::size_t value, std::size_t alignment) noexcept;

void* aligned_allocate(std::size_t bytes, std::size_t alignment);
void aligned_release(void* pointer, std::size_t alignment) noexcept;

template<typename T>
T* allocate_objects(std::size_t count) {
  static_assert(std::is_trivially_destructible_v<T>);
  if (count == 0) {
    return nullptr;
  }
  const std::size_t alignment = alignof(T) > simd_alignment ? alignof(T) : simd_alignment;
  auto* raw = static_cast<T*>(aligned_allocate(count * sizeof(T), alignment));
  for (std::size_t i = 0; i < count; ++i) {
    new (raw + i) T();
  }
  return raw;
}

template<typename T>
void release_objects(T* pointer, std::size_t count) noexcept {
  if (pointer == nullptr) {
    return;
  }
  for (std::size_t i = 0; i < count; ++i) {
    pointer[i].~T();
  }
  const std::size_t alignment = alignof(T) > simd_alignment ? alignof(T) : simd_alignment;
  aligned_release(pointer, alignment);
}

}
