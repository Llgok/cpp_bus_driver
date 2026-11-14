/*
 * @Description: cpp_bus_driver 日志接口
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace cpp_bus_driver {
class Logger {
 public:
  enum class LogLevel : uint8_t {
    kDebug,
    kInfo,
    kWarning,
    kError,
    kNone,
  };

  static void SetMinimumLogLevel(LogLevel level);
  static LogLevel GetMinimumLogLevel();
  static bool ShouldLog(LogLevel level);

  void LogMessage(LogLevel level, const char* file_name, size_t line_number,
      const char* format, ...);
};
}  // namespace cpp_bus_driver
