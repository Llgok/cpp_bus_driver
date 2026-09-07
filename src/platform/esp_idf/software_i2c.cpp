/*
 * @Description: ESP-IDF 后端软件模拟 I2C 总线驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-09-04 11:03:14
 * @License: GPL 3.0
 */
#include "bus/i2c/software_i2c.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
#include "driver/gpio.h"

namespace cpp_bus_driver {
bool SoftwareI2c::Init(uint32_t freq_hz, uint16_t address) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (freq_hz == 0 || freq_hz > kMaximumFrequencyHz ||
      (address != kNoDeviceAddress && address > 0x7F) || sda_ == scl_ ||
      !GPIO_IS_VALID_OUTPUT_GPIO(sda_) || !GPIO_IS_VALID_OUTPUT_GPIO(scl_)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid SoftwareI2c configuration\n");
    return false;
  }

  if (!initialized_) {
    // 先预置高电平，再使能开漏输出，避免初始化时主动拉低总线。
    gpio_cleanup_required_ = true;
    if (!ReleaseLines() ||
        !SetGpioMode(sda_, GpioMode::kInputOutputOd, GpioStatus::kPullup) ||
        !SetGpioMode(scl_, GpioMode::kInputOutputOd, GpioStatus::kPullup)) {
      ResetPins();
      return false;
    }
    initialized_ = true;
  }

  // 用商和余数向上取整，避免大频率加法溢出，并保证半周期至少为 1 us。
  half_period_us_ = 500000U / freq_hz + (500000U % freq_hz != 0);
  address_ = address;
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config address: %#X\n", address);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config sda_: %d\n", sda_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config scl_: %d\n", scl_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config freq_hz: %lu hz\n",
      static_cast<unsigned long>(freq_hz));
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config half_period_us_: %lu us\n",
      static_cast<unsigned long>(half_period_us_));
  return true;
}

bool SoftwareI2c::Deinit(bool delete_bus) {
  std::lock_guard<std::mutex> lock(mutex_);
  bool result = true;
  if (gpio_cleanup_required_) {
    result = FinishTransaction(true);
    if (delete_bus) {
      result = ResetPins() && result;
    }
  }
  initialized_ = false;
  address_ = kNoDeviceAddress;
  return result;
}

bool SoftwareI2c::Read(uint8_t* data, size_t length) {
  std::lock_guard<std::mutex> lock(mutex_);
  return Transfer(nullptr, 0, data, length);
}

bool SoftwareI2c::Write(const uint8_t* data, size_t length) {
  std::lock_guard<std::mutex> lock(mutex_);
  return Transfer(data, length, nullptr, 0);
}

bool SoftwareI2c::WriteRead(const uint8_t* write_data, size_t write_length,
    uint8_t* read_data, size_t read_length) {
  std::lock_guard<std::mutex> lock(mutex_);
  return Transfer(write_data, write_length, read_data, read_length);
}

bool SoftwareI2c::Transfer(const uint8_t* write_data, size_t write_length,
    uint8_t* read_data, size_t read_length) {
  if (!initialized_ || address_ == kNoDeviceAddress) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SoftwareI2c not initialized or no device address bound\n");
    return false;
  }
  if ((write_length != 0 && write_data == nullptr) ||
      (read_length != 0 && read_data == nullptr)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Invalid I2C buffer\n");
    return false;
  }
  if (write_length == 0 && read_length == 0) {
    return true;
  }

  bool result = StartCondition();
  if (result && write_length != 0) {
    result = WriteByte(static_cast<uint8_t>(address_ << 1));
    for (size_t i = 0; result && i < write_length; ++i) {
      result = WriteByte(write_data[i]);
    }
  }
  if (result && read_length != 0) {
    if (write_length != 0) {
      result = StartCondition();
    }
    if (result) {
      result = WriteByte(static_cast<uint8_t>((address_ << 1) | 1));
    }
    for (size_t i = 0; result && i < read_length; ++i) {
      uint8_t value = 0;
      // ACK 为低电平，最后一个字节发送 NACK 结束读取。
      result = ReadByte(value) && WriteBit(i == read_length - 1);
      if (result) {
        read_data[i] = value;
      }
    }
  }

  if (!result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SoftwareI2c transfer failed (address: %#X)\n", address_);
  }
  return FinishTransaction(result);
}

bool SoftwareI2c::Probe(uint16_t address) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_ || address > 0x7F) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SoftwareI2c not initialized or invalid probe address\n");
    return false;
  }
  const bool result =
      StartCondition() && WriteByte(static_cast<uint8_t>(address << 1));
  return FinishTransaction(result);
}

bool SoftwareI2c::RecoverBus() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SoftwareI2c not initialized\n");
    return false;
  }
  if (!ReleaseLines() || !RaiseClock()) {
    return FinishTransaction(false);
  }

  // 从设备可能仍在发送一个字节，提供剩余时钟让其释放 SDA。
  for (uint8_t i = 0; i < 9 && !GpioRead(sda_); ++i) {
    if (!GpioWrite(scl_, 0)) {
      return FinishTransaction(false);
    }
    DelayUs(half_period_us_);
    if (!RaiseClock()) {
      return FinishTransaction(false);
    }
  }

  // 即使没有活动事务，也要补发停止信号结束从设备的状态机。
  transaction_active_ = true;
  return FinishTransaction(true);
}

bool SoftwareI2c::StartCondition() {
  // 重复起始时先在 SCL 低电平期间释放 SDA。
  if (!GpioWrite(sda_, 1)) {
    return false;
  }
  DelayUs(half_period_us_);
  if (!RaiseClock()) {
    return false;
  }
  if (!GpioRead(sda_)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SoftwareI2c bus not idle (SDA GPIO %d is low)\n", sda_);
    return false;
  }
  if (!GpioWrite(sda_, 0)) {
    return false;
  }
  transaction_active_ = true;
  DelayUs(half_period_us_);
  return GpioWrite(scl_, 0);
}

bool SoftwareI2c::StopCondition() {
  bool result = GpioWrite(scl_, 0);
  result &= GpioWrite(sda_, 0);
  if (result) {
    DelayUs(half_period_us_);
    result = RaiseClock();
  }
  if (result) {
    result = GpioWrite(sda_, 1);
    DelayUs(half_period_us_);
    result = GpioRead(sda_) && GpioRead(scl_) && result;
  }
  transaction_active_ = false;
  if (!result) {
    ReleaseLines();
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SoftwareI2c stop failed; bus may still be held low\n");
  }
  return result;
}

bool SoftwareI2c::FinishTransaction(bool success) {
  const bool cleanup = transaction_active_ ? StopCondition() : ReleaseLines();
  return success && cleanup;
}

bool SoftwareI2c::RaiseClock() {
  if (!GpioWrite(scl_, 1)) {
    return false;
  }
  const int64_t start_us = GetSystemTimeUs();
  while (!GpioRead(scl_)) {
    if (GetSystemTimeUs() - start_us >= kClockStretchTimeoutUs) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "SoftwareI2c SCL timeout (GPIO %d remains low)\n", scl_);
      return false;
    }
    DelayUs(1);
  }
  DelayUs(half_period_us_);
  return true;
}

bool SoftwareI2c::WriteBit(bool high) {
  if (!GpioWrite(sda_, high)) {
    return false;
  }
  DelayUs(half_period_us_);
  if (!RaiseClock()) {
    return false;
  }
  if (high && !GpioRead(sda_)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SoftwareI2c SDA conflict (GPIO %d remains low)\n", sda_);
    return false;
  }
  return GpioWrite(scl_, 0);
}

bool SoftwareI2c::ReadBit(bool& high) {
  if (!GpioWrite(sda_, 1)) {
    return false;
  }
  DelayUs(half_period_us_);
  if (!RaiseClock()) {
    return false;
  }
  const bool value = GpioRead(sda_);
  if (!GpioWrite(scl_, 0)) {
    return false;
  }
  high = value;
  return true;
}

bool SoftwareI2c::WriteByte(uint8_t data) {
  for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
    if (!WriteBit((data & mask) != 0)) {
      return false;
    }
  }
  bool nack = true;
  return ReadBit(nack) && !nack;
}

bool SoftwareI2c::ReadByte(uint8_t& data) {
  uint8_t value = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    bool high = false;
    if (!ReadBit(high)) {
      return false;
    }
    value = static_cast<uint8_t>((value << 1) | high);
  }
  data = value;
  return true;
}

bool SoftwareI2c::ReleaseLines() {
  bool result = GpioWrite(sda_, 1);
  result &= GpioWrite(scl_, 1);
  return result;
}

bool SoftwareI2c::ResetPins() {
  bool result = ResetGpio(sda_);
  result &= ResetGpio(scl_);
  if (result) {
    gpio_cleanup_required_ = false;
  }
  return result;
}
}  // namespace cpp_bus_driver
#endif
