#include "aetherq/memory/aligned_alloc.hpp"

#include <cstdlib>
#include <memory>

namespace aetherq {

bool is_aligned(const void* pointer, std::size_t alignment) noexcept {
  return reinterpret_cast<std::uintptr_t>(pointer) % alignment == 0;
}

std::size_t round_up(std::size_t value, std::size_t alignment) noexcept {
  if (alignment == 0) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

void* aligned_allocate(std::size_t bytes, std::size_t alignment) {
  const std::size_t effective = bytes == 0 ? alignment : bytes;
  void* pointer = std::aligned_alloc(alignment, round_up(effective, alignment));
  if (pointer == nullptr) {
    throw std::bad_alloc();
  }
  return pointer;
}

void aligned_release(void* pointer, std::size_t alignment) noexcept {
  static_cast<void>(alignment);
  std::free(pointer);
}

}
