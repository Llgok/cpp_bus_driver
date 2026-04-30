/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-04-30 13:44:59
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
class HardwareI2c1 final : public BusI2cGuide {
 public:
  explicit HardwareI2c1(int32_t sda, int32_t scl, i2c_port_t port = I2C_NUM_0)
      : sda_(sda), scl_(scl), port_(port) {}

  bool Init(uint32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE,
      uint16_t address = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;
  bool Read(uint8_t* data, size_t length) override;
  bool Write(const uint8_t* data, size_t length) override;
  bool WriteRead(const uint8_t* write_data, size_t write_length,
      uint8_t* read_data, size_t read_length) override;

  bool Probe(const uint16_t address) override;

  bool set_bus_handle(i2c_master_bus_handle_t bus_handle);

  i2c_master_bus_handle_t bus_handle();

 private:
  int32_t sda_, scl_;
  i2c_port_t port_;
  uint16_t address_ = CPP_BUS_DRIVER_DEFAULT_VALUE;
  uint32_t freq_hz_ = CPP_BUS_DRIVER_DEFAULT_VALUE;
  i2c_master_dev_handle_t device_handle_ = nullptr;
  i2c_master_bus_handle_t bus_handle_ = nullptr;
};
#endif
}  // namespace cpp_bus_driver
