/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-04-29 16:48:10
 * @License: GPL 3.0
 */
#include "hardware_spi.h"

namespace cpp_bus_driver {
bool HardwareSpi::Init(int32_t freq_hz, int32_t cs) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  if (bus_init_flag_ && device_init_flag_) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "HardwareSpi has been initialized\n");
    return true;
  }

#endif

  if (freq_hz == CPP_BUS_DRIVER_DEFAULT_VALUE) {
    freq_hz = CPP_BUS_DRIVER_DEFAULT_SPI_FREQ_HZ;
  }

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  if (flags_ == CPP_BUS_DRIVER_DEFAULT_VALUE) {
    flags_ = static_cast<uint32_t>(NULL);
  }
#endif

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config mosi_: %d\n", mosi_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config sclk_: %d\n", sclk_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config miso_: %d\n", miso_);
  LogMessage(
      LogLevel::kInfo, __FILE__, __LINE__, "HardwareSpi config cs: %d\n", cs);

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config port_: %d\n", port_);
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config port_ address: %#X\n", port_);
#endif

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config mode_: %d\n", mode_);

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config clock_source_: %d\n", clock_source_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config flags_: %d\n", flags_);
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config bit_order_: %d\n", bit_order_);
#endif

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config freq_hz: %d hz\n", freq_hz);

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF

  if (!bus_init_flag_) {
    const spi_bus_config_t bus_config = {
        .mosi_io_num = mosi_,
        .miso_io_num = miso_,
        .sclk_io_num = sclk_,
        .quadwp_io_num = -1,  // WP引脚不设置，这个引脚配置Quad SPI的时候才有用
        .quadhd_io_num = -1,  // HD引脚不设置，这个引脚配置Quad SPI的时候才有用
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .data_io_default_level = 0,
        .max_transfer_sz = 0,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = static_cast<uint32_t>(NULL),
    };

    esp_err_t result = spi_bus_initialize(port_, &bus_config, SPI_DMA_CH_AUTO);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "spi_bus_initialize failed (error code: %#X)\n", result);
      return false;
    }

    bus_init_flag_ = true;
  }

  if (!device_init_flag_) {
    const spi_device_interface_config_t device_config = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,  // 无虚拟位
        .mode = mode_,
        .clock_source = clock_source_,  // 时钟源
        .duty_cycle_pos = 128,          // 50% 占空比
        .cs_ena_pretrans =
            1,  // 在数据传输开始之前，片选信号（CS）应该提前多少个SPI位周期被激活
        .cs_ena_posttrans =
            1,  // 在数据传输结束后，片选信号（CS）应该保持激活状态多少个SPI位周期
        .clock_speed_hz = freq_hz,
        .input_delay_ns = 0,  // 无输入延迟
        .sample_point = spi_sampling_point_t::SPI_SAMPLING_POINT_PHASE_0,
        .spics_io_num = cs,
        .flags = flags_,  // 标志，可以填入SPI_DEVICE_BIT_LSBFIRST等信息
        .queue_size = 1,
        .pre_cb = NULL,   // 无传输前回调
        .post_cb = NULL,  // 无传输后回调
    };
    esp_err_t result = spi_bus_add_device(port_, &device_config, &spi_device_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "spi_bus_add_device failed (error code: %#X)\n", result);
      return false;
    }

    device_init_flag_ = true;
  }

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  spi_handle_ = std::make_unique<SPIClass>(port_, static_cast<uint8_t>(miso_),
      static_cast<uint8_t>(sclk_), static_cast<uint8_t>(mosi_));
  GpioMode(cs, GpioMode::kOutput);
  spi_settings_ = SPISettings(freq_hz, bit_order_, mode_);

  spi_handle_->begin();

#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
  return false;
#endif

  freq_hz_ = freq_hz;
  cs_ = cs;

  return true;
}

bool HardwareSpi::Deinit(bool delete_bus) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  bool result = true;

  if (device_init_flag_) {
    esp_err_t ret = spi_bus_remove_device(spi_device_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "spi_bus_remove_device failed (error code: %#X)\n", ret);
      result = false;
    } else {
      spi_device_ = nullptr;
      device_init_flag_ = false;
    }
  }

  if (delete_bus && bus_init_flag_) {
    esp_err_t ret = spi_bus_free(port_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "spi_bus_free failed (error code: %#X)\n", ret);
      result = false;
    } else {
      bus_init_flag_ = false;
    }
  } else {
    bus_init_flag_ = false;
  }

  return result;
#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
  return false;
#endif
}

bool HardwareSpi::Write(const void* data, size_t byte) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  spi_transaction_t buffer = {
      .flags = static_cast<uint32_t>(NULL),
      .cmd = 0,
      .addr = 0,
      .length = byte * 8,
      .rxlength = 0,
      .override_freq_hz = 0,
      .user = (void*)0,
      .tx_buffer = data,
      .rx_buffer = NULL,
  };

  esp_err_t result = spi_device_polling_transmit(spi_device_, &buffer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "spi_device_polling_transmit failed (error code: %#X)\n", result);
    return false;
  }

  return true;

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  spi_handle_->beginTransaction(spi_settings_);
  GpioWrite(cs_, 0);
  spi_handle_->transfer(const_cast<void*>(data), byte);
  GpioWrite(cs_, 1);
  spi_handle_->endTransaction();

  return true;
#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
  return false;
#endif
}

bool HardwareSpi::Read(void* data, size_t byte) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  spi_transaction_t buffer = {
      .flags = static_cast<uint32_t>(NULL),
      .cmd = 0,
      .addr = 0,
      .length = byte * 8,
      .rxlength = 0,
      .override_freq_hz = 0,
      .user = (void*)0,
      .tx_buffer = NULL,
      .rx_buffer = data,
  };

  esp_err_t result = spi_device_polling_transmit(spi_device_, &buffer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "spi_device_polling_transmit failed (error code: %#X)\n", result);
    return false;
  }

  return true;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  uint8_t buffer[byte] = {0};

  spi_handle_->beginTransaction(spi_settings_);
  GpioWrite(cs_, 0);
  spi_handle_->transfer(buffer, data, byte);
  GpioWrite(cs_, 1);
  spi_handle_->endTransaction();

  return true;
#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Read failed\n");
  return false;
#endif
}

bool HardwareSpi::WriteRead(
    const void* write_data, void* read_data, size_t data_byte) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  spi_transaction_t buffer = {
      .flags = static_cast<uint32_t>(NULL),
      .cmd = 0,
      .addr = 0,
      .length = data_byte * 8,
      .rxlength = 0,
      .override_freq_hz = 0,
      .user = (void*)0,
      .tx_buffer = write_data,
      .rx_buffer = read_data,
  };

  esp_err_t result = spi_device_polling_transmit(spi_device_, &buffer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "spi_device_polling_transmit failed (error code: %#X)\n", result);
    return false;
  }

  return true;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  spi_handle_->beginTransaction(spi_settings_);
  GpioWrite(cs_, 0);
  spi_handle_->transfer(write_data, read_data, data_byte);
  GpioWrite(cs_, 1);
  spi_handle_->endTransaction();

  return true;
#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteRead failed\n");
  return false;
#endif
}
}  // namespace cpp_bus_driver
