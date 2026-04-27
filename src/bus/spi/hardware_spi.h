
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-04-25 17:23:55
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace cpp_bus_driver {
class HardwareSpi final : public BusSpiGuide {
 public:
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  explicit HardwareSpi(int32_t mosi, int32_t sclk,
      int32_t miso = CPP_BUS_DRIVER_DEFAULT_VALUE,
      spi_host_device_t port = SPI2_HOST, uint8_t mode = 0,
      uint32_t flags = CPP_BUS_DRIVER_DEFAULT_VALUE,
      spi_clock_source_t clock_source = SPI_CLK_SRC_DEFAULT)
      : mosi_(mosi),
        sclk_(sclk),
        miso_(miso),
        port_(port),
        mode_(mode),
        flags_(flags),
        clock_source_(clock_source) {}
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  HardwareSpi(int32_t mosi, int32_t sclk,
      int32_t miso = CPP_BUS_DRIVER_DEFAULT_VALUE,
      NRF_SPIM_Type* port = NRF_SPIM_3, uint8_t mode = 0,
      BitOrder bit_order = BitOrder::kMsbfirst)
      : mosi_(mosi),
        sclk_(sclk),
        miso_(miso),
        port_(port),
        mode_(mode),
        bit_order_(bit_order) {}
#endif

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE,
      int32_t cs = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Write(const void* data, size_t byte) override;
  bool Read(void* data, size_t byte) override;
  bool WriteRead(
      const void* write_data, void* read_data, size_t data_byte) override;

  void set_bus_init_flag(bool enable) { bus_init_flag_ = enable; }

 private:
  int32_t mosi_, sclk_, miso_, cs_, freq_hz_;

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  spi_host_device_t port_;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  NRF_SPIM_Type* port_;
#endif

  uint8_t mode_;

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  uint32_t flags_;
  spi_clock_source_t clock_source_;

  bool bus_init_flag_ = false;
  bool device_init_flag_ = false;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  BitOrder bit_order_;
#endif

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  spi_device_handle_t spi_device_;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  std::unique_ptr<SPIClass> spi_handle_;
  SPISettings spi_settings_;
#endif
};
}  // namespace cpp_bus_driver