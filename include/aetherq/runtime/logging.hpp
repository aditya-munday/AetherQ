#pragma once

#include <cstdint>
#include <mutex>
#include <ostream>
#include <string_view>

namespace aetherq {

enum class LogLevel : std::uint8_t { debug, info, warn, error };

std::string_view log_level_name(LogLevel level) noexcept;

class Logger {
public:
  explicit Logger(LogLevel threshold, std::ostream& sink);

  void log(LogLevel level, std::string_view message);
  void set_threshold(LogLevel level) noexcept;
  [[nodiscard]] LogLevel threshold() const noexcept;

private:
  mutable std::mutex mutex_;
  LogLevel threshold_;
  std::ostream* sink_;
};

Logger& default_logger();

}
