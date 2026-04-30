
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-04-22 15:01:23
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
class HardwareUart final : public BusUartGuide {
 public:
  explicit HardwareUart(int32_t tx, int32_t rx,
      uart_port_t port = uart_port_t::UART_NUM_1,
      int32_t rts = CPP_BUS_DRIVER_DEFAULT_VALUE,
      int32_t cts = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : tx_(tx), rx_(rx), port_(port), rts_(rts), cts_(cts) {}

  bool Init(int32_t baud_rate = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  int32_t Read(void* data, uint32_t length) override;
  int32_t Write(const void* data, size_t length) override;

  size_t GetRxBufferLength() override;
  bool ClearRxBufferData() override;
  bool SetBaudRate(uint32_t baud_rate) override;
  uint32_t GetBaudRate() override;
  bool Deinit() override;

 private:
  static constexpr uint16_t kUartRxMaxSize = 1024 * 2;

  int32_t tx_, rx_;
  uart_port_t port_;
  int32_t rts_, cts_, baud_rate_;
  bool init_flag_ = false;
};
#endif
}  // namespace cpp_bus_driver
