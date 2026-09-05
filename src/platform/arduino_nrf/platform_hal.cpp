/*
 * @Description: Arduino nRF 平台硬件抽象实现
 * @Author: LILYGO_L
 * @Date: 2026-09-04 10:14:25
 * @LastEditTime: 2026-09-05 14:57:25
 * @License: GPL 3.0
 */
#include "platform/platform_hal.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52

#include "Arduino.h"
#include "nrf_gpio.h"

namespace cpp_bus_driver {
bool PlatformHal::SetGpioMode(int32_t pin, GpioMode mode, GpioStatus status) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  switch (mode) {
    case GpioMode::kDisable:
      nrf_gpio_cfg_default(pin);
      break;
    case GpioMode::kInput:
      switch (status) {
        case GpioStatus::kDisable:
          pinMode(pin, 0x0);
          break;
        case GpioStatus::kPullup:
          pinMode(pin, 0x2);
          break;
        case GpioStatus::kPulldown:
          pinMode(pin, 0x3);
          break;

        default:
          pinMode(pin, 0x0);
          break;
      }
      break;
    case GpioMode::kOutput:
      pinMode(pin, 0x1);
      break;

    default:
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
      nrf_gpio_cfg_default(pin);
      return false;
  }

  return true;
}

bool PlatformHal::GpioWrite(int32_t pin, bool value) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  digitalWrite(pin, value);
  return true;
}

bool PlatformHal::GpioRead(int32_t pin) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  return digitalRead(pin);
}

bool PlatformHal::ResetGpio(int32_t pin) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  nrf_gpio_cfg_default(pin);
  return true;
}

void PlatformHal::DelayMs(uint32_t value) { delay(value); }

void PlatformHal::DelayUs(uint32_t value) { delayMicroseconds(value); }

int64_t PlatformHal::GetSystemTimeUs() const {
  return static_cast<int64_t>(micros());
}

int64_t PlatformHal::GetSystemTimeMs() const {
  return static_cast<int64_t>(millis());
}

}  // namespace cpp_bus_driver

#endif
