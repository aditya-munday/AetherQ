#include "aetherq/runtime/result.hpp"

namespace aetherq {

std::string_view error_code_name(ErrorCode code) noexcept {
  switch (code) {
    case ErrorCode::invalid_argument:
      return "invalid_argument";
    case ErrorCode::invalid_config:
      return "invalid_config";
    case ErrorCode::io_error:
      return "io_error";
    case ErrorCode::schema_violation:
      return "schema_violation";
    case ErrorCode::numerical_failure:
      return "numerical_failure";
    case ErrorCode::concurrency_error:
      return "concurrency_error";
    case ErrorCode::cancelled:
      return "cancelled";
    case ErrorCode::internal:
      return "internal";
  }
  return "unknown";
}

Error::Error(ErrorCode code, std::string message) : code_(code), message_(std::move(message)) {}

ErrorCode Error::code() const noexcept {
  return code_;
}

const std::string& Error::message() const noexcept {
  return message_;
}

std::string Error::to_string() const {
  return std::string(error_code_name(code_)) + ": " + message_;
}

ErrorException::ErrorException(Error error)
    : std::runtime_error(error.to_string()), error_(std::move(error)) {}

const Error& ErrorException::error() const noexcept {
  return error_;
}

}
