#include "aetherq/runtime/version.hpp"

namespace aetherq {

namespace {
constexpr std::string_view kVersion = "0.1.0";
}

std::string_view version() noexcept {
  return kVersion;
}
std::string_view git_commit() noexcept {
  return AETHERQ_GIT_COMMIT;
}
std::string_view git_dirty() noexcept {
  return AETHERQ_GIT_DIRTY;
}
std::string_view build_type() noexcept {
  return AETHERQ_BUILD_TYPE;
}

std::string_view compiler_id() noexcept {
#if defined(__clang__)
  return "clang";
#elif defined(__GNUC__)
  return "gcc";
#elif defined(_MSC_VER)
  return "msvc";
#else
  return "unknown";
#endif
}

std::string_view compiler_version() noexcept {
#if defined(__clang__)
  return __clang_version__;
#elif defined(__GNUC__)
  return __VERSION__;
#else
  return "unknown";
#endif
}

}
