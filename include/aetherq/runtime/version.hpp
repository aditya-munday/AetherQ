#pragma once

#include <string_view>

namespace aetherq {

std::string_view version() noexcept;
std::string_view git_commit() noexcept;
std::string_view git_dirty() noexcept;
std::string_view build_type() noexcept;
std::string_view compiler_id() noexcept;
std::string_view compiler_version() noexcept;

}
