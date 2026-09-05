/*
 * @Description: ESP-IDF 后端硬件 SPI 总线驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-09-04 10:14:25
 * @LastEditTime: 2026-09-05 14:57:37
 * @License: GPL 3.0
 */
#include "bus/spi/hardware_spi.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32

#include "driver/spi_master.h"

namespace cpp_bus_driver {
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
bool HardwareSpi::InitBus() {
  if (shared_bus_provider_ != nullptr) {
    if (!shared_bus_provider_->InitBus()) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "Init shared spi bus failed\n");
      return false;
    }
    bus_init_state_.store(BusInitState::kReady);
    delete_bus_on_deinit_ = false;
    return true;
  }

  if (bus_init_state_.load() == BusInitState::kReady) {
    return true;
  }

  BusInitState expected = BusInitState::kNotStarted;
  if (!bus_init_state_.compare_exchange_strong(
          expected, BusInitState::kInitializing)) {
    const int64_t start_time_ms = GetSystemTimeMs();
    while (bus_init_state_.load() == BusInitState::kInitializing) {
      if (GetSystemTimeMs() - start_time_ms >= kBusInitWaitTimeoutMs) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__,
            "Wait spi bus init timeout\n");
        return false;
      }
      DelayMs(1);
    }

    const bool ready = bus_init_state_.load() == BusInitState::kReady;
    return ready;
  }

  const spi_bus_config_t bus_config = {
      .mosi_io_num = mosi_,
      .miso_io_num = miso_,
      .sclk_io_num = sclk_,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .data_io_default_level = 0,
      .max_transfer_sz = 0,
      .flags = SPICOMMON_BUSFLAG_MASTER,
      .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
      .intr_flags = 0,
  };

  esp_err_t result = spi_bus_initialize(port_, &bus_config, SPI_DMA_CH_AUTO);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "spi_bus_initialize failed (error code: %#X)\n", result);
    bus_init_state_.store(BusInitState::kNotStarted);
    return false;
  }

  delete_bus_on_deinit_ = true;
  bus_init_state_.store(BusInitState::kReady);

  return true;
}

void HardwareSpi::set_bus_init_flag(bool enable) {
  shared_bus_provider_.reset();
  bus_init_state_.store(
      enable ? BusInitState::kReady : BusInitState::kNotStarted);
  delete_bus_on_deinit_ = false;
}

bool HardwareSpi::Init(int32_t freq_hz, int32_t cs) {
  if (freq_hz <= 0) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Invalid bus frequency\n");
    return false;
  }
  if (bus_init_state_.load() == BusInitState::kReady && device_init_flag_) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HardwareSpi has been initialized\n");
    return true;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config mosi_: %d\n", mosi_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config sclk_: %d\n", sclk_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config miso_: %d\n", miso_);
  LogMessage(
      LogLevel::kInfo, __FILE__, __LINE__, "HardwareSpi config cs: %d\n", cs);

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config port_: %d\n", port_);

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config mode_: %d\n", mode_);

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config clock_source_: %d\n", clock_source_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config flags_: %d\n", flags_);

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config freq_hz: %d hz\n", freq_hz);

  const bool had_bus = bus_init_state_.load() == BusInitState::kReady;
  if (!InitBus()) {
    return false;
  }
  const bool created_bus = !had_bus && delete_bus_on_deinit_;

  if (!device_init_flag_) {
    const spi_device_interface_config_t device_config = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = mode_,
        .clock_source = clock_source_,
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 1,
        .cs_ena_posttrans = 1,
        .clock_speed_hz = freq_hz,
        .input_delay_ns = 0,
        .sample_point = spi_sampling_point_t::SPI_SAMPLING_POINT_PHASE_0,
        .spics_io_num = cs,
        .flags = flags_,
        .queue_size = 1,
        .pre_cb = nullptr,
        .post_cb = nullptr,
    };
    esp_err_t result = spi_bus_add_device(port_, &device_config, &spi_device_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "spi_bus_add_device failed (error code: %#X)\n", result);
      Deinit(created_bus);
      return false;
    }

    device_init_flag_ = true;
  }

  cs_ = cs;

  return true;
}

bool HardwareSpi::Deinit(bool delete_bus) {
  bool result = true;

  if (device_init_flag_) {
    esp_err_t ret = spi_bus_remove_device(spi_device_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "spi_bus_remove_device failed (error code: %#X)\n", ret);
      result = false;
    } else {
      spi_device_ = nullptr;
      device_init_flag_ = false;
      if (cs_ != kPinNotConnected) {
        result &= ResetGpio(cs_);
      }
      cs_ = kPinNotConnected;
    }
  }

  if (delete_bus && bus_init_state_.load() == BusInitState::kReady) {
    if (!delete_bus_on_deinit_) {
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "Skip deleting external spi bus\n");
      return result;
    }

    esp_err_t ret = spi_bus_free(port_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "spi_bus_free failed (error code: %#X)\n", ret);
      result = false;
    } else {
      bus_init_state_.store(BusInitState::kNotStarted);
      delete_bus_on_deinit_ = false;
      if (mosi_ != kPinNotConnected) {
        result &= ResetGpio(mosi_);
      }
      if (miso_ != kPinNotConnected) {
        result &= ResetGpio(miso_);
      }
      if (sclk_ != kPinNotConnected) {
        result &= ResetGpio(sclk_);
      }
    }
  }

  return result;
}

bool HardwareSpi::Write(const void* data, size_t byte) {
  spi_transaction_t buffer = {
      .flags = 0,
      .cmd = 0,
      .addr = 0,
      .length = byte * 8,
      .rxlength = 0,
      .override_freq_hz = 0,
      .user = nullptr,
      .tx_buffer = data,
      .rx_buffer = nullptr,
  };

  esp_err_t result = spi_device_polling_transmit(spi_device_, &buffer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "spi_device_polling_transmit failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareSpi::Read(void* data, size_t byte) {
  spi_transaction_t buffer = {
      .flags = 0,
      .cmd = 0,
      .addr = 0,
      .length = byte * 8,
      .rxlength = 0,
      .override_freq_hz = 0,
      .user = nullptr,
      .tx_buffer = nullptr,
      .rx_buffer = data,
  };

  esp_err_t result = spi_device_polling_transmit(spi_device_, &buffer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "spi_device_polling_transmit failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareSpi::WriteRead(
    const void* write_data, void* read_data, size_t data_byte) {
  spi_transaction_t buffer = {
      .flags = 0,
      .cmd = 0,
      .addr = 0,
      .length = data_byte * 8,
      .rxlength = 0,
      .override_freq_hz = 0,
      .user = nullptr,
      .tx_buffer = write_data,
      .rx_buffer = read_data,
  };

  esp_err_t result = spi_device_polling_transmit(spi_device_, &buffer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "spi_device_polling_transmit failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}
#endif

}  // namespace cpp_bus_driver

#endif
