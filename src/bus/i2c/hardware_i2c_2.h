/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-04-30 13:45:04
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace cpp_bus_driver {
class HardwareI2c2 final : public BusI2cGuide {
 public:
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  explicit HardwareI2c2(int32_t sda, int32_t scl, i2c_port_t port = I2C_NUM_0)
      : sda_(sda), scl_(scl), port_(port) {}
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  explicit HardwareI2c2(int32_t sda, int32_t scl, TwoWire* i2c_handle = &Wire)
      : sda_(sda), scl_(scl), i2c_handle_(i2c_handle) {}
#endif

  bool Init(uint32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE,
      uint16_t address = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;
  bool Read(uint8_t* data, size_t length) override;
  bool Write(const uint8_t* data, size_t length) override;
  bool WriteRead(const uint8_t* write_data, size_t write_length,
      uint8_t* read_data, size_t read_length) override;

  bool Probe(const uint16_t address) override;

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  i2c_cmd_handle_t CmdLinkCreate() override;
  bool StartTransmit(
      i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en = true) override;
  bool Read(i2c_cmd_handle_t cmd_handle, uint8_t* data, size_t data_len,
      i2c_ack_type_t ack = I2C_MASTER_LAST_NACK) override;
  bool Write(
      i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en = true) override;
  bool Write(i2c_cmd_handle_t cmd_handle, const uint8_t* data, size_t data_len,
      bool ack_en = true) override;
  bool StopTransmit(i2c_cmd_handle_t cmd_handle) override;
#endif

 private:
  int32_t sda_, scl_;

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  i2c_port_t port_;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  TwoWire* i2c_handle_;
#endif

  int16_t address_ = CPP_BUS_DRIVER_DEFAULT_VALUE;
  int32_t freq_hz_ = CPP_BUS_DRIVER_DEFAULT_VALUE;
};
}  // namespace cpp_bus_driver
