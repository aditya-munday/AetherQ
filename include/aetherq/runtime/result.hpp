#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace aetherq {

enum class ErrorCode {
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
  Error(ErrorCode code, std::string message);

  [[nodiscard]] ErrorCode code() const noexcept;
  [[nodiscard]] const std::string& message() const noexcept;
  [[nodiscard]] std::string to_string() const;

private:
  ErrorCode code_;
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
  Result(T value) : data_(std::move(value)) {}
  Result(Error error) : data_(std::move(error)) {}

  [[nodiscard]] bool ok() const noexcept {
    return std::holds_alternative<T>(data_);
  }
  explicit operator bool() const noexcept {
    return ok();
  }

  [[nodiscard]] T& value() & {
    return std::get<T>(data_);
  }
  [[nodiscard]] const T& value() const& {
    return std::get<T>(data_);
  }
  [[nodiscard]] T&& value() && {
    return std::get<T>(std::move(data_));
  }

  [[nodiscard]] const Error& error() const {
    return std::get<Error>(data_);
  }

  T& expect(std::string_view context) & {
    if (!ok()) {
      throw ErrorException(Error(error().code(), std::string(context) + ": " + error().message()));
    }
    return value();
  }

private:
  std::variant<T, Error> data_;
};

template<>
class Result<void> {
public:
  Result() = default;
  Result(Error error) : error_(std::move(error)), ok_(false) {}

  [[nodiscard]] bool ok() const noexcept {
    return ok_;
  }
  explicit operator bool() const noexcept {
    return ok_;
  }
  [[nodiscard]] const Error& error() const {
    return error_;
  }

  void expect(std::string_view context) const {
    if (!ok_) {
      throw ErrorException(Error(error_.code(), std::string(context) + ": " + error_.message()));
    }
  }

private:
  Error error_{ErrorCode::internal, std::string("no error")};
  bool ok_{true};
};

inline Result<void> ok_result() {
  return Result<void>();
}

}
