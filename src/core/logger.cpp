/*
 * @Description: cpp_bus_driver 日志实现
 * @Author: LILYGO_L
 * @Date: 2026-09-04 13:51:51
 * @LastEditTime: 2026-09-05 14:56:30
 * @License: GPL 3.0
 */
#include "logger.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <new>

#include "cpp_bus_driver_config.h"

namespace cpp_bus_driver {
namespace {

#if defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_DEBUG)
constexpr Logger::LogLevel kDefaultMinimumLogLevel = Logger::LogLevel::kDebug;
#elif defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_INFO)
constexpr Logger::LogLevel kDefaultMinimumLogLevel = Logger::LogLevel::kInfo;
#elif defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_WARNING)
constexpr Logger::LogLevel kDefaultMinimumLogLevel = Logger::LogLevel::kWarning;
#elif defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_ERROR)
constexpr Logger::LogLevel kDefaultMinimumLogLevel = Logger::LogLevel::kError;
#elif defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_NONE)
constexpr Logger::LogLevel kDefaultMinimumLogLevel = Logger::LogLevel::kNone;
#else
constexpr Logger::LogLevel kDefaultMinimumLogLevel = Logger::LogLevel::kInfo;
#endif

std::atomic<Logger::LogLevel> g_minimum_log_level{kDefaultMinimumLogLevel};

const char* LogLevelName(Logger::LogLevel level) {
  switch (level) {
    case Logger::LogLevel::kDebug:
      return "Debug";
    case Logger::LogLevel::kInfo:
      return "Info";
    case Logger::LogLevel::kWarning:
      return "Warning";
    case Logger::LogLevel::kError:
      return "Error";
    case Logger::LogLevel::kNone:
      return "None";
    default:
      return "Unknown";
  }
}

}  // namespace

void Logger::SetMinimumLogLevel(LogLevel level) {
  if (level > LogLevel::kNone) {
    level = LogLevel::kNone;
  }
  g_minimum_log_level.store(level, std::memory_order_relaxed);
}

Logger::LogLevel Logger::GetMinimumLogLevel() {
  return g_minimum_log_level.load(std::memory_order_relaxed);
}

bool Logger::ShouldLog(LogLevel level) {
  if (level > LogLevel::kError) {
    return false;
  }
  const LogLevel minimum_level = GetMinimumLogLevel();
  return minimum_level != LogLevel::kNone && level >= minimum_level;
}

void Logger::LogMessage(LogLevel level, const char* file_name,
    size_t line_number, const char* format, ...) {
  if (!ShouldLog(level)) {
    return;
  }

  va_list args;
  va_start(args, format);
  // 内存不足时跳过日志，避免错误处理路径再次触发分配异常。
  std::unique_ptr<char[]> buffer(new (std::nothrow) char[kMaxLogBufferSize]);
  if (buffer == nullptr) {
    va_end(args);
    return;
  }
  snprintf(buffer.get(), kMaxLogBufferSize,
      "[cpp_bus_driver log][%s]->[%s][%u line]: %s", LogLevelName(level),
      file_name, static_cast<unsigned int>(line_number), format);
  vprintf(buffer.get(), args);
  va_end(args);
}
}  // namespace cpp_bus_driver
