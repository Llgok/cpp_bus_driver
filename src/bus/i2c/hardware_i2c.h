/*
 * @Description: 跨平台硬件 I2C 总线驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-09-04 10:45:43
 * @LastEditTime: 2026-09-04 10:45:43
 * @License: GPL 3.0
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "bus/bus_base.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
#include "driver/i2c_master.h"
#elif CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52
#include "Wire.h"
#endif

namespace cpp_bus_driver {
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
class HardwareI2c final : public I2cBusBase {
 public:
  explicit HardwareI2c(int32_t sda, int32_t scl, i2c_port_t port = I2C_NUM_0)
      : sda_(sda), scl_(scl), port_(port) {}
  explicit HardwareI2c(const std::shared_ptr<HardwareI2c>& bus)
      : sda_(bus == nullptr ? kPinNotConnected : bus->sda_),
        scl_(bus == nullptr ? kPinNotConnected : bus->scl_),
        port_(bus == nullptr ? I2C_NUM_0 : bus->port_),
        shared_bus_provider_(bus) {}
  bool InitBus(uint32_t freq_hz = kDefaultFrequencyHz);
  bool Init(uint32_t freq_hz = kDefaultFrequencyHz,
      uint16_t address = kNoDeviceAddress) override;
  bool Deinit(bool delete_bus = true) override;
  bool Read(uint8_t* data, size_t length) override;
  bool Write(const uint8_t* data, size_t length) override;
  bool WriteRead(const uint8_t* write_data, size_t write_length,
      uint8_t* read_data, size_t read_length) override;

  bool Probe(const uint16_t address) override;
  bool set_bus_handle(i2c_master_bus_handle_t bus_handle);
  i2c_master_bus_handle_t bus_handle();

 private:
  // 默认总线时钟，单位 Hz。
  static constexpr uint32_t kDefaultFrequencyHz = 100000;

  static constexpr int kDefaultWaitTimeoutMs = 1000;

  int32_t sda_, scl_;
  i2c_port_t port_;
  uint16_t address_ = kNoDeviceAddress;
  i2c_master_dev_handle_t device_handle_ = nullptr;
  i2c_master_bus_handle_t bus_handle_ = nullptr;

  enum class BusInitState : uint8_t {
    kNotStarted,
    kInitializing,
    kReady,
  };

  static constexpr int64_t kBusInitWaitTimeoutMs = 1000;

  std::atomic<BusInitState> bus_init_state_{BusInitState::kNotStarted};
  std::shared_ptr<HardwareI2c> shared_bus_provider_;
  bool delete_bus_on_deinit_ = false;
};
#elif CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52
class HardwareI2c final : public I2cBusBase {
 public:
  explicit HardwareI2c(int32_t sda, int32_t scl, TwoWire* i2c_handle = &Wire)
      : sda_(sda), scl_(scl), i2c_handle_(i2c_handle) {}

  bool Init(uint32_t freq_hz = kDefaultFrequencyHz,
      uint16_t address = kNoDeviceAddress) override;
  bool Deinit(bool delete_bus = true) override;
  bool Read(uint8_t* data, size_t length) override;
  bool Write(const uint8_t* data, size_t length) override;
  bool WriteRead(const uint8_t* write_data, size_t write_length,
      uint8_t* read_data, size_t read_length) override;

  bool Probe(const uint16_t address) override;

 private:
  // 默认总线时钟，单位 Hz。
  static constexpr uint32_t kDefaultFrequencyHz = 100000;

  int32_t sda_, scl_;
  TwoWire* i2c_handle_;
  uint16_t address_ = kNoDeviceAddress;
};
#endif

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
class LegacyHardwareI2c final : public I2cBusBase {
 public:
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  explicit LegacyHardwareI2c(
      int32_t sda, int32_t scl, i2c_port_t port = I2C_NUM_0)
      : sda_(sda), scl_(scl), port_(port) {}
#endif

  bool Init(uint32_t freq_hz = kDefaultFrequencyHz,
      uint16_t address = kNoDeviceAddress) override;
  bool Deinit(bool delete_bus = true) override;
  bool Read(uint8_t* data, size_t length) override;
  bool Write(const uint8_t* data, size_t length) override;
  bool WriteRead(const uint8_t* write_data, size_t write_length,
      uint8_t* read_data, size_t read_length) override;

  bool Probe(const uint16_t address) override;

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  i2c_cmd_handle_t CreateCommandLink() override;
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
  // 默认总线时钟，单位 Hz。
  static constexpr uint32_t kDefaultFrequencyHz = 100000;

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  static constexpr int kDefaultWaitTimeoutMs = 1000;
#endif

  int32_t sda_, scl_;

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  i2c_port_t port_;
#endif

  uint16_t address_ = kNoDeviceAddress;
};
#endif
}  // namespace cpp_bus_driver
