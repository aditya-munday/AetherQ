#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace aetherq {

enum class ErrorCode : std::uint8_t {
  invalid_argument,
  invalid_config,
  io_error,
  schema_violation,
  numerical_failure,
  concurrency_error,
  cancelled,
  internal
};

std::string_view error_code_name(ErrorCode code) noexcept;

class Error {
public:
  Error(ErrorCode code, std::string message) : code_(code), message_(std::move(message)) {}
  Error(const Error& other) = default;
  Error& operator=(const Error& other) = default;
  Error(Error&& other) noexcept = default;
  Error& operator=(Error&& other) noexcept = default;
  ~Error() = default;

  [[nodiscard]] ErrorCode code() const noexcept {
    return code_;
  }
  [[nodiscard]] const std::string& message() const noexcept {
    return message_;
  }
  [[nodiscard]] std::string to_string() const;

private:
  ErrorCode code_{ErrorCode::internal};
  std::string message_;
};

class ErrorException : public std::runtime_error {
public:
  explicit ErrorException(Error error);

  [[nodiscard]] const Error& error() const noexcept;

private:
  Error error_;
};

template<typename T>
class Result {
public:
  Result(T value) : value_(std::move(value)) {}
  Result(Error error) : error_(std::move(error)) {}

  [[nodiscard]] bool ok() const noexcept {
    return value_.has_value();
  }
  explicit operator bool() const noexcept {
    return ok();
  }

  [[nodiscard]] T& value() & {
    if (!value_) {
      throw ErrorException(Error(error().code(), std::string("no value: ") + error().message()));
    }
    return *value_;
  }
  [[nodiscard]] const T& value() const& {
    if (!value_) {
      throw ErrorException(Error(error().code(), std::string("no value: ") + error().message()));
    }
    return *value_;
  }
  [[nodiscard]] T&& value() && {
    if (!value_) {
      throw ErrorException(Error(error().code(), std::string("no value: ") + error().message()));
    }
    return std::move(*value_);
  }

  [[nodiscard]] const Error& error() const {
    if (!error_) {
      throw ErrorException(Error(ErrorCode::internal, "result holds a value, not an error"));
    }
    return *error_;
  }

  T& expect(std::string_view context) & {
    if (!ok()) {
      throw ErrorException(Error(error().code(), std::string(context) + ": " + error().message()));
    }
    return value();
  }

private:
  std::optional<T> value_;
  std::optional<Error> error_;
};

template<>
class Result<void> {
public:
  Result() = default;
  Result(Error error) : error_(std::move(error)) {}

  [[nodiscard]] bool ok() const noexcept {
    return !error_.has_value();
  }
  explicit operator bool() const noexcept {
    return ok();
  }
  [[nodiscard]] const Error& error() const {
    if (!error_) {
      throw ErrorException(Error(ErrorCode::internal, "result holds a value, not an error"));
    }
    return *error_;
  }

  void expect(std::string_view context) const {
    if (!ok()) {
      throw ErrorException(Error(error().code(), std::string(context) + ": " + error().message()));
    }
  }

private:
  std::optional<Error> error_;
};

inline Result<void> ok_result() {
  return {};
}

}
