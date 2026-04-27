
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-04-17 13:59:01
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
class HardwareQspi final : public BusQspiGuide {
 public:
  explicit HardwareQspi(int32_t data0, int32_t data1, int32_t data2,
      int32_t data3, int32_t sclk, spi_host_device_t port = SPI2_HOST,
      int8_t mode = 0, spi_clock_source_t clock_source = SPI_CLK_SRC_DEFAULT,
      uint32_t flags = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : data0_(data0),
        data1_(data1),
        data2_(data2),
        data3_(data3),
        sclk_(sclk),
        port_(port),
        mode_(mode),
        clock_source_(clock_source),
        flags_(flags) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE,
      int32_t cs = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  // bool Write(const uint16_t cmd, const uint64_t addr, const uint8_t *data =
  // NULL, size_t byte = 0, uint32_t flags = static_cast<uint32_t>(NULL))
  // override; bool Write(const void *data, size_t byte, bool cs_keep_active)
  // override;
  bool Write(const void* data, size_t byte,
      uint32_t flags = static_cast<uint32_t>(NULL),
      bool cs_keep_active = false) override;

  bool SetCs(bool value);

 private:
  // 这里设置最大传输尺寸
  // esp32s3的dma最大尺寸是32k
  static constexpr int32_t kQspiMaxTransferSize = 32 * 1024;

  int32_t data0_, data1_, data2_, data3_, sclk_, cs_, freq_hz_;
  spi_host_device_t port_;
  uint8_t mode_;
  spi_clock_source_t clock_source_;
  uint32_t flags_;
  size_t max_transfer_size_;
  spi_device_handle_t spi_device_;
};
#endif
}  // namespace cpp_bus_driver