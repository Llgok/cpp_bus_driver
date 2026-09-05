/*
 * @Description: 软件模拟 I2C 总线驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-09-03 17:45:24
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "bus/bus_base.h"

namespace cpp_bus_driver {
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
class SoftwareI2c final : public I2cBusBase {
 public:
  // i2c通信中，应答(ack)是低电平(0)，非应答(nack)是高电平(1)
  enum class AckBit {
    kAck = 0,
    kNack,
  };

  explicit SoftwareI2c(int32_t sda, int32_t scl) : sda_(sda), scl_(scl) {}

  bool Init(uint32_t freq_hz = kDefaultFrequencyHz,
      uint16_t address = kNoDeviceAddress) override;
  bool Deinit(bool delete_bus = true) override;

  bool StartTransmit() override;
  bool Read(uint8_t* data, size_t length) override;
  bool Write(const uint8_t* data, size_t length) override;
  bool WriteRead(const uint8_t* write_data, size_t write_length,
      uint8_t* read_data, size_t read_length) override;
  bool StopTransmit() override;

  bool Probe(const uint16_t address) override;
  bool WriteByte(uint8_t data);
  bool ReadByte(uint8_t* data);
  bool WaitAck();

  /**
   * @brief 写应答
   * @param ack 要发送的应答位
   * @return 写入成功返回 true，失败返回 false
   */
  bool WriteAck(AckBit ack);

 private:
  // 默认总线时钟，单位 Hz。
  static constexpr uint32_t kDefaultFrequencyHz = 100000;

  int32_t sda_, scl_;
  uint16_t address_ = kNoDeviceAddress;
  uint32_t transmit_delay_us_ = 0;
};
#endif
}  // namespace cpp_bus_driver
