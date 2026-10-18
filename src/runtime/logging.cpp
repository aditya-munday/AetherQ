#include "aetherq/runtime/logging.hpp"

#include <iostream>
#include <string>

namespace aetherq {

std::string_view log_level_name(LogLevel level) noexcept {
  switch (level) {
    case LogLevel::debug:
      return "debug";
    case LogLevel::info:
      return "info";
    case LogLevel::warn:
      return "warn";
    case LogLevel::error:
      return "error";
  }
  return "unknown";
}

Logger::Logger(LogLevel threshold, std::ostream& sink) : threshold_(threshold), sink_(&sink) {}

void Logger::log(LogLevel level, std::string_view message) {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (level < threshold_) {
    return;
  }
  *sink_ << "[" << log_level_name(level) << "] " << message << '\n';
}

void Logger::set_threshold(LogLevel level) noexcept {
  const std::lock_guard<std::mutex> lock(mutex_);
  threshold_ = level;
}

LogLevel Logger::threshold() const noexcept {
  const std::lock_guard<std::mutex> lock(mutex_);
  return threshold_;
}

Logger& default_logger() {
  static Logger logger(LogLevel::warn, std::cerr);
  return logger;
}

}
