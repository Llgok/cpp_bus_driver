/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-04-20 17:56:23
 * @License: GPL 3.0
 */
#include "hardware_qspi.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
bool HardwareQspi::Init(int32_t freq_hz, int32_t cs) {
  if (freq_hz == CPP_BUS_DRIVER_DEFAULT_VALUE) {
    freq_hz = CPP_BUS_DRIVER_DEFAULT_QSPI_FREQ_HZ;
  }

  if (flags_ == CPP_BUS_DRIVER_DEFAULT_VALUE) {
    flags_ = SPI_DEVICE_HALFDUPLEX;
  }

  if (cs_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    cs_ = cs;
    SetGpioMode(cs_, GpioMode::kOutput, GpioStatus ::kPullup);
    SetCs(1);
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config data0_: %d\n", data0_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config data1_: %d\n", data1_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config data2_: %d\n", data2_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config data3_: %d\n", data3_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config sclk_: %d\n", sclk_);
  LogMessage(
      LogLevel::kInfo, __FILE__, __LINE__, "HardwareQspi config cs: %d\n", cs_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config port_: %d\n", port_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config mode_: %d\n", mode_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config clock_source_: %d\n", clock_source_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config flags_: %d\n", flags_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config freq_hz: %d hz\n", freq_hz);

  const spi_bus_config_t bus_config = {
      .data0_io_num = data0_,
      .data1_io_num = data1_,
      .sclk_io_num = sclk_,
      .data2_io_num = data2_,
      .data3_io_num = data3_,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .data_io_default_level = 0,
      .max_transfer_sz = kQspiMaxTransferSize,
      .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_QUAD,
      .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
      .intr_flags = static_cast<uint32_t>(NULL),
  };

  esp_err_t result = spi_bus_initialize(port_, &bus_config, SPI_DMA_CH_AUTO);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "spi_bus_initialize failed (error code: %#X)\n", result);
    return false;
  }

  const spi_device_interface_config_t device_config = {
      .command_bits = 0,
      .address_bits = 0,
      .dummy_bits = 0,  // 无虚拟位
      .mode = mode_,
      .clock_source = clock_source_,  // 默认时钟源
      .duty_cycle_pos = 128,          // 50% 占空比
      .cs_ena_pretrans =
          0,  // 在数据传输开始之前，片选信号（CS）应该提前多少个SPI位周期被激活
      .cs_ena_posttrans =
          0,  // 在数据传输结束后，片选信号（CS）应该保持激活状态多少个SPI位周期
      .clock_speed_hz = freq_hz,
      .input_delay_ns = 0,  // 无输入延迟
      .sample_point = spi_sampling_point_t::SPI_SAMPLING_POINT_PHASE_0,
      .spics_io_num = -1,
      .flags = flags_,  // 标志，可以填入SPI_DEVICE_BIT_LSBFIRST等信息
      .queue_size = 1,
      .pre_cb = NULL,   // 无传输前回调
      .post_cb = NULL,  // 无传输后回调
  };

  result = spi_bus_add_device(port_, &device_config, &spi_device_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "spi_bus_add_device failed (error code: %#X)\n", result);
    return false;
  }

  size_t buffer = 0;
  result = spi_bus_get_max_transaction_len(port_, &buffer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "spi_bus_get_max_transaction_len failed (error code: %#X)\n", result);
    max_transfer_size_ = kQspiMaxTransferSize;
  } else {
    max_transfer_size_ = buffer;
  }
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareQspi config max_transfer_size_: %d\n", max_transfer_size_);

  freq_hz_ = freq_hz;

  return true;
}

bool HardwareQspi::Write(
    const void* data, size_t byte, uint32_t flags, bool cs_keep_active) {
  spi_transaction_t buffer = {
      .flags = flags,
      .cmd = 0,
      .addr = 0,
      .length = byte * 8,
      .rxlength = 0,
      .override_freq_hz = 0,
      .user = (void*)0,
      .tx_buffer = data,
      .rx_buffer = NULL,
  };

  if (byte > max_transfer_size_) {
    const uint8_t* buffer_data_ptr = static_cast<const uint8_t*>(data);
    size_t buffer_send_count = byte / max_transfer_size_;
    size_t buffer_remaining_size = byte % max_transfer_size_;

    buffer.length = max_transfer_size_ * 8;

    SetCs(0);
    for (size_t i = 0; i < buffer_send_count; i++) {
      buffer.tx_buffer = buffer_data_ptr;

      esp_err_t result = spi_device_polling_transmit(spi_device_, &buffer);
      if (result != ESP_OK) {
        SetCs(1);
        LogMessage(LogLevel::kBus, __FILE__, __LINE__,
            "spi_device_polling_transmit failed (error code: %#X)\n", result);
        return false;
      }

      buffer_data_ptr += max_transfer_size_;
    }
    if (buffer_remaining_size > 0) {
      buffer.tx_buffer = buffer_data_ptr;
      buffer.length = buffer_remaining_size * 8;

      esp_err_t result = spi_device_polling_transmit(spi_device_, &buffer);
      if (result != ESP_OK) {
        SetCs(1);
        LogMessage(LogLevel::kBus, __FILE__, __LINE__,
            "spi_device_polling_transmit failed (error code: %#X)\n", result);
        return false;
      }
    }

    if (!cs_keep_active) {
      SetCs(1);
    }
  } else {
    SetCs(0);
    esp_err_t result = spi_device_polling_transmit(spi_device_, &buffer);
    if (result != ESP_OK) {
      SetCs(1);
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "spi_device_polling_transmit failed (error code: %#X)\n", result);
      return false;
    }
    if (!cs_keep_active) {
      SetCs(1);
    }
  }

  return true;
}

bool HardwareQspi::SetCs(bool value) {
  if (cs_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    if (!GpioWrite(cs_, value)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
      return false;
    }
  }

  return true;
}
#endif
}  // namespace cpp_bus_driver