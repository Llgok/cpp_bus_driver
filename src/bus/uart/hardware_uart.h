/*
 * @Description: ESP-IDF 硬件 UART 总线驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-09-05 14:56:45
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "bus/bus_base.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
#include "driver/uart.h"
#endif

namespace cpp_bus_driver {
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
class HardwareUart final : public UartBusBase {
 public:
  explicit HardwareUart(int32_t tx, int32_t rx,
      uart_port_t port = uart_port_t::UART_NUM_1,
      int32_t rts = kPinNotConnected, int32_t cts = kPinNotConnected)
      : tx_(tx), rx_(rx), port_(port), rts_(rts), cts_(cts) {}

  bool Init(int32_t baud_rate = kDefaultBaudRate) override;
  int32_t Read(void* data, uint32_t length) override;
  int32_t Write(const void* data, size_t length) override;

  size_t GetRxBufferLength() override;
  bool ClearRxBufferData() override;
  bool SetBaudRate(uint32_t baud_rate) override;
  uint32_t GetBaudRate() override;
  bool Deinit() override;

 private:
  // 默认 UART 波特率。
  static constexpr int32_t kDefaultBaudRate = 115200;

  static constexpr int kDefaultWaitTimeoutMs = 1000;
  static constexpr uint16_t kUartRxMaxSize = 1024 * 2;

  int32_t tx_, rx_;
  uart_port_t port_;
  int32_t rts_, cts_;
  bool init_flag_ = false;
};
#endif
}  // namespace cpp_bus_driver
