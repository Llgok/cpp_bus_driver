/*
 * @Description: Arduino nRF 平台硬件 I2C 总线驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-09-05 14:57:20
 * @License: GPL 3.0
 */
#include "bus/i2c/hardware_i2c.h"

#include <limits>

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52

namespace cpp_bus_driver {
bool HardwareI2c::Init(uint32_t freq_hz, uint16_t address) {
  if (freq_hz == 0 ||
      freq_hz > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Invalid I2C frequency\n");
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c config address: %#X\n", address);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c config sda_: %d\n", sda_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c config scl_: %d\n", scl_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c config freq_hz: %d hz\n", freq_hz);

  if (address == kNoDeviceAddress) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "address is null\n");
  }

  i2c_handle_->setPins(static_cast<uint8_t>(sda_), static_cast<uint8_t>(scl_));
  i2c_handle_->setClock(freq_hz);
  i2c_handle_->begin();

  address_ = address;

  return true;
}

bool HardwareI2c::Deinit(bool delete_bus) {
  i2c_handle_->end();

  return true;
}

bool HardwareI2c::Read(uint8_t* data, size_t length) {
  const size_t read_count = i2c_handle_->requestFrom(address_, length);
  if (read_count != length) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "requestFrom failed\n");
    return false;
  }
  for (size_t i = 0; i < read_count; i++) {
    data[i] = static_cast<uint8_t>(i2c_handle_->read());
  }

  return true;
}

bool HardwareI2c::Write(const uint8_t* data, size_t length) {
  i2c_handle_->beginTransmission(address_);

  size_t result = i2c_handle_->write(data, length);
  if (result == 0) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  } else if (result != length) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "buffer is full (written: %u, expected: %u)\n",
        static_cast<unsigned int>(result), static_cast<unsigned int>(length));
    return false;
  }

  result = i2c_handle_->endTransmission();
  switch (result) {
    case 1:
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "data too long\n");
      return false;
    case 2:
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "nack on transmit of address\n");
      return false;
    case 3:
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "nack on transmit of data\n");
      return false;
    case 4:
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "other error\n");
      return false;
    default:
      break;
  }

  return true;
}

bool HardwareI2c::WriteRead(const uint8_t* write_data, size_t write_length,
    uint8_t* read_data, size_t read_length) {
  i2c_handle_->beginTransmission(address_);

  size_t result = i2c_handle_->write(write_data, write_length);
  if (result == 0) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  } else if (result != write_length) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "buffer is full (written: %u, expected: %u)\n",
        static_cast<unsigned int>(result),
        static_cast<unsigned int>(write_length));
    return false;
  }

  result = i2c_handle_->endTransmission();
  switch (result) {
    case 1:
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "data too long\n");
      return false;
    case 2:
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "nack on transmit of address\n");
      return false;
    case 3:
    case 4:
      // 兼容现有行为：数据无应答或其他错误时仍尝试后续读取。
    default:
      break;
  }

  const size_t read_count = i2c_handle_->requestFrom(address_, read_length);
  if (read_count != read_length) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "requestFrom failed\n");
    return false;
  }
  for (size_t i = 0; i < read_count; i++) {
    read_data[i] = static_cast<uint8_t>(i2c_handle_->read());
  }

  return true;
}

bool HardwareI2c::Probe(const uint16_t address) {
  i2c_handle_->beginTransmission(address);
  uint8_t result = i2c_handle_->endTransmission();
  switch (result) {
    case 1:
      return false;
    case 2:
      return false;
    case 3:
      return false;
    case 4:
      return false;
    default:
      break;
  }

  return true;
}

}  // namespace cpp_bus_driver

#endif
