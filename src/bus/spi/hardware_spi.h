/*
 * @Description: 跨平台硬件 SPI 总线驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-09-05 14:56:43
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
#include "driver/spi_master.h"
#elif CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52
#include "Arduino.h"
#include "SPI.h"
#endif

namespace cpp_bus_driver {
class HardwareSpi final : public SpiBusBase {
 public:
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  explicit HardwareSpi(int32_t mosi, int32_t sclk,
      int32_t miso = kPinNotConnected, spi_host_device_t port = SPI2_HOST,
      uint8_t mode = 0, uint32_t flags = kDefaultDeviceFlags,
      spi_clock_source_t clock_source = SPI_CLK_SRC_DEFAULT)
      : mosi_(mosi),
        sclk_(sclk),
        miso_(miso),
        port_(port),
        mode_(mode),
        flags_(flags),
        clock_source_(clock_source) {}
  explicit HardwareSpi(const std::shared_ptr<HardwareSpi>& bus,
      uint8_t mode = 0, uint32_t flags = kDefaultDeviceFlags,
      spi_clock_source_t clock_source = SPI_CLK_SRC_DEFAULT)
      : mosi_(bus == nullptr ? kPinNotConnected : bus->mosi_),
        sclk_(bus == nullptr ? kPinNotConnected : bus->sclk_),
        miso_(bus == nullptr ? kPinNotConnected : bus->miso_),
        port_(bus == nullptr ? SPI2_HOST : bus->port_),
        mode_(mode),
        flags_(flags),
        clock_source_(clock_source),
        shared_bus_provider_(bus) {}
#elif CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52
  HardwareSpi(int32_t mosi, int32_t sclk, int32_t miso = kPinNotConnected,
      NRF_SPIM_Type* port = NRF_SPIM3, uint8_t mode = 0,
      BitOrder bit_order = MSBFIRST)
      : mosi_(mosi),
        sclk_(sclk),
        miso_(miso),
        port_(port),
        mode_(mode),
        bit_order_(bit_order) {}
#endif

  bool Init(int32_t freq_hz = kDefaultFrequencyHz,
      int32_t cs = kPinNotConnected) override;
  bool Write(const void* data, size_t byte) override;
  bool Read(void* data, size_t byte) override;
  bool WriteRead(
      const void* write_data, void* read_data, size_t data_byte) override;
  bool Deinit(bool delete_bus = true) override;

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  bool InitBus();
  void set_bus_init_flag(bool enable);
#endif

 private:
  // 默认总线时钟，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 10000000;

  // 默认不启用额外的 SPI 设备标志。
  static constexpr uint32_t kDefaultDeviceFlags = 0;

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  enum class BusInitState : uint8_t {
    kNotStarted,
    kInitializing,
    kReady,
  };

  static constexpr int64_t kBusInitWaitTimeoutMs = 1000;
#endif

  int32_t mosi_, sclk_, miso_;
  int32_t cs_ = kPinNotConnected;

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  spi_host_device_t port_;
#elif CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52
  NRF_SPIM_Type* port_;
#endif

  uint8_t mode_;

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  uint32_t flags_;
  spi_clock_source_t clock_source_;

  std::atomic<BusInitState> bus_init_state_{BusInitState::kNotStarted};
  bool device_init_flag_ = false;
  bool delete_bus_on_deinit_ = false;
  std::shared_ptr<HardwareSpi> shared_bus_provider_;
#elif CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52
  BitOrder bit_order_;
#endif

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  spi_device_handle_t spi_device_ = nullptr;
#elif CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52
  std::unique_ptr<SPIClass> spi_handle_;
  SPISettings spi_settings_;
#endif
};
}  // namespace cpp_bus_driver
