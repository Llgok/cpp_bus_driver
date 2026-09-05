/*
 * @Description: cpp_bus_driver 平台硬件抽象接口
 * @Author: LILYGO_L
 * @Date: 2026-09-04 12:28:47
 * @LastEditTime: 2026-09-05 14:56:34
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>

#include "../core/logger.h"
#include "cpp_bus_driver_config.h"

namespace cpp_bus_driver {
class PlatformHal : public Logger {
 public:
  // 可选 GPIO 未连接时使用的引脚编号。
  static constexpr int kPinNotConnected = -1;

  enum class InterruptMode {
    kDisable,
    kRising,
    kFalling,
    kChange,
    kOnLow,
    kOnHigh,
  };

  enum class GpioMode {
    kDisable,
    kInput,
    kOutput,
    kOutputOd,
    kInputOutputOd,
    kInputOutput,
  };

  enum class GpioStatus {
    kDisable,
    kPullup,
    kPulldown,
  };

  bool SetGpioMode(
      int32_t pin, GpioMode mode, GpioStatus status = GpioStatus::kDisable);
  bool GpioWrite(int32_t pin, bool value);
  bool GpioRead(int32_t pin);
  bool ResetGpio(int32_t pin);
  void DelayMs(uint32_t value);
  void DelayUs(uint32_t value);
  int64_t GetSystemTimeUs() const;
  int64_t GetSystemTimeMs() const;

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  bool InitGpioInterrupt(uint32_t pin, InterruptMode mode,
      void (*interrupt)(void* arg), void* args = nullptr,
      GpioStatus status = GpioStatus::kDisable);
  bool DeinitGpioInterrupt(uint32_t pin);
#endif
};
}  // namespace cpp_bus_driver
