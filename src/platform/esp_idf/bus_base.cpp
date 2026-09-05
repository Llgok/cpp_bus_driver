/*
 * @Description: ESP-IDF I2C 命令链兼容接口实现
 * @Author: LILYGO_L
 * @Date: 2026-09-04 10:14:25
 * @LastEditTime: 2026-09-05 14:57:27
 * @License: GPL 3.0
 */
#include "bus/bus_base.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32

#include "driver/i2c.h"

namespace cpp_bus_driver {
i2c_cmd_handle_t I2cBusBase::CreateCommandLink() {
  LogMessage(
      LogLevel::kError, __FILE__, __LINE__, "CreateCommandLink failed\n");
  return nullptr;
}

bool I2cBusBase::StartTransmit(
    i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en) {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "StartTransmit failed\n");
  return false;
}

bool I2cBusBase::Write(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en) {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
  return false;
}

bool I2cBusBase::Write(i2c_cmd_handle_t cmd_handle, const uint8_t* data,
    size_t data_len, bool ack_en) {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
  return false;
}

bool I2cBusBase::Read(i2c_cmd_handle_t cmd_handle, uint8_t* data,
    size_t data_len, i2c_ack_type_t ack) {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
  return false;
}

bool I2cBusBase::StopTransmit(i2c_cmd_handle_t cmd_handle) {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "StopTransmit failed\n");
  return false;
}

bool I2cBusBase::StartTransmit() {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "StartTransmit failed\n");
  return false;
}

bool I2cBusBase::StopTransmit() {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "StopTransmit failed\n");
  return false;
}

}  // namespace cpp_bus_driver

#endif
