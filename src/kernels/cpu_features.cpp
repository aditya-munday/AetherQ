#include "aetherq/kernels/dispatch.hpp"

namespace aetherq {

CpuFeatures detect_cpu_features() noexcept {
  CpuFeatures features;
#if defined(__x86_64__) || defined(__i386__)
  features.sse42 = __builtin_cpu_supports("sse4.2") != 0;
  features.avx2 = __builtin_cpu_supports("avx2") != 0;
  features.avx512f = __builtin_cpu_supports("avx512f") != 0;
#elif defined(__aarch64__) || defined(__ARM_NEON)
  features.neon = true;
#endif
  return features;
}

std::string_view best_backend_name(const CpuFeatures& features) noexcept {
  if (features.avx512f) {
    return "x86-avx512";
  }
  if (features.avx2) {
    return "x86-avx2";
  }
  if (features.sse42) {
    return "x86-sse42";
  }
  if (features.neon) {
    return "arm-neon";
  }
  return "portable";
}

}
