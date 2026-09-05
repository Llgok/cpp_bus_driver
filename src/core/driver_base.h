/*
 * @Description: cpp_bus_driver 驱动公共基类接口
 * @Author: LILYGO_L
 * @Date: 2026-09-04 13:51:51
 * @LastEditTime: 2026-09-04 15:08:17
 * @License: GPL 3.0
 */
#pragma once

#include "platform/platform_hal.h"

namespace cpp_bus_driver {
class DriverBase : public PlatformHal {
 public:
  DriverBase() = default;
  virtual ~DriverBase() = default;

 protected:
  enum class InitSequenceFormat {
    kDelayMs,
    kWriteC8,
    kWriteC8ByteData,
    kWriteC8D8,
    kWriteC8R24,
    kWriteC8R24D8,
    kWriteC16D8,
  };

  enum class ByteOrder {
    kBig,
    kLittle,
  };
};
}  // namespace cpp_bus_driver
