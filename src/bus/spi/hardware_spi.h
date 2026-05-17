/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-05-17 19:49:15
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace cpp_bus_driver {
class HardwareSpi final : public BusSpiGuide {
 public:
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  explicit HardwareSpi(int32_t mosi, int32_t sclk,
      int32_t miso = kDefaultValue,
      spi_host_device_t port = SPI2_HOST, uint8_t mode = 0,
      uint32_t flags = kDefaultValue,
      spi_clock_source_t clock_source = SPI_CLK_SRC_DEFAULT)
      : mosi_(mosi),
        sclk_(sclk),
        miso_(miso),
        port_(port),
        mode_(mode),
        flags_(flags),
        clock_source_(clock_source) {}
  explicit HardwareSpi(const std::shared_ptr<HardwareSpi>& bus,
      uint8_t mode = 0, uint32_t flags = kDefaultValue,
      spi_clock_source_t clock_source = SPI_CLK_SRC_DEFAULT)
      : mosi_(bus == nullptr ? kDefaultValue : bus->mosi_),
        sclk_(bus == nullptr ? kDefaultValue : bus->sclk_),
        miso_(bus == nullptr ? kDefaultValue : bus->miso_),
        port_(bus == nullptr ? SPI2_HOST : bus->port_),
        mode_(mode),
        flags_(flags),
        clock_source_(clock_source),
        shared_bus_provider_(bus) {}
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  HardwareSpi(int32_t mosi, int32_t sclk,
      int32_t miso = kDefaultValue,
      NRF_SPIM_Type* port = NRF_SPIM_3, uint8_t mode = 0,
      BitOrder bit_order = BitOrder::kMsbfirst)
      : mosi_(mosi),
        sclk_(sclk),
        miso_(miso),
        port_(port),
        mode_(mode),
        bit_order_(bit_order) {}
#endif

  bool Init(int32_t freq_hz = kDefaultValue,
      int32_t cs = kDefaultValue) override;
  bool Write(const void* data, size_t byte) override;
  bool Read(void* data, size_t byte) override;
  bool WriteRead(
      const void* write_data, void* read_data, size_t data_byte) override;
  bool Deinit(bool delete_bus = true) override;

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  bool InitBus();
  void set_bus_init_flag(bool enable);
#endif

 private:
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  enum class BusInitState : uint8_t {
    kNotStarted,
    kInitializing,
    kReady,
  };

  static constexpr int64_t kBusInitWaitTimeoutMs = 1000;
#endif

  int32_t mosi_, sclk_, miso_;
  int32_t cs_ = kDefaultValue;
  int32_t freq_hz_ = kDefaultValue;

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  spi_host_device_t port_;
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  NRF_SPIM_Type* port_;
#endif

  uint8_t mode_;

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  uint32_t flags_;
  spi_clock_source_t clock_source_;

  std::atomic<BusInitState> bus_init_state_{BusInitState::kNotStarted};
  bool device_init_flag_ = false;
  bool delete_bus_on_deinit_ = false;
  std::shared_ptr<HardwareSpi> shared_bus_provider_;
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  BitOrder bit_order_;
#endif

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  spi_device_handle_t spi_device_ = nullptr;
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  std::unique_ptr<SPIClass> spi_handle_;
  SPISettings spi_settings_;
#endif
};
}  // namespace cpp_bus_driver
