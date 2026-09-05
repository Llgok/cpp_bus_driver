/*
 * @Description: cpp_bus_driver 日志接口
 * @Author: LILYGO_L
 * @Date: 2026-09-04 12:28:47
 * @LastEditTime: 2026-09-04 13:51:51
 * @License: GPL 3.0
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

 private:
  static constexpr uint16_t kMaxLogBufferSize = 1024;
};
}  // namespace cpp_bus_driver
