/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-04-24 10:01:30
 * @License: GPL 3.0
 */
#include "hardware_sdio.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
bool HardwareSdio::Init(int32_t freq_hz) {
  if (freq_hz == CPP_BUS_DRIVER_DEFAULT_VALUE) {
    freq_hz = CPP_BUS_DRIVER_DEFAULT_SDIO_FREQ_HZ;
  } else if ((freq_hz != SDMMC_FREQ_DEFAULT) ||
             (freq_hz != SDMMC_FREQ_HIGHSPEED) ||
             (freq_hz != SDMMC_FREQ_PROBING) || (freq_hz != SDMMC_FREQ_52M) ||
             (freq_hz != SDMMC_FREQ_26M)) {
    freq_hz = CPP_BUS_DRIVER_DEFAULT_SDIO_FREQ_HZ;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config port_: %d\n", port_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config freq_hz: %d\n", freq_hz);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config clk_: %d\n", clk_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config cmd_: %d\n", cmd_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config d0_: %d\n", d0_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config d1_: %d\n", d1_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config d2_: %d\n", d2_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config d3_: %d\n", d3_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config d4_: %d\n", d4_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config d5_: %d\n", d5_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config d6_: %d\n", d6_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config d7_: %d\n", d7_);

  if (d7_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    width_ = 8;
  } else if (d6_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    width_ = 7;
  } else if (d5_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    width_ = 6;
  } else if (d4_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    width_ = 5;
  } else if (d3_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    width_ = 4;
  } else if (d2_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    width_ = 3;
  } else if (d1_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    width_ = 2;
  } else {
    width_ = 1;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSdio config width_: %d\n", width_);

  sdmmc_slot_config_t sdmmc_slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  sdmmc_slot_config.clk = static_cast<gpio_num_t>(clk_);
  sdmmc_slot_config.cmd = static_cast<gpio_num_t>(cmd_);
  sdmmc_slot_config.d0 = static_cast<gpio_num_t>(d0_);
  sdmmc_slot_config.d1 = static_cast<gpio_num_t>(d1_);
  sdmmc_slot_config.d2 = static_cast<gpio_num_t>(d2_);
  sdmmc_slot_config.d3 = static_cast<gpio_num_t>(d3_);
  sdmmc_slot_config.d4 = static_cast<gpio_num_t>(d4_);
  sdmmc_slot_config.d5 = static_cast<gpio_num_t>(d5_);
  sdmmc_slot_config.d6 = static_cast<gpio_num_t>(d6_);
  sdmmc_slot_config.d7 = static_cast<gpio_num_t>(d7_);
  sdmmc_slot_config.width = width_;

  esp_err_t result = sdmmc_host_init();
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_host_init failed (error code: %#X)\n", result);
    return false;
  }

  result = sdmmc_host_init_slot(static_cast<int>(port_), &sdmmc_slot_config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_host_init_slot failed (error code: %#X)\n", result);
  }

  sdio_handle_ = std::make_unique<sdmmc_card_t>();
  if (sdio_handle_ == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  sdmmc_host_t sdmmc_host = SDMMC_HOST_DEFAULT();
  sdmmc_host.flags |=
      SDMMC_HOST_FLAG_ALLOC_ALIGNED_BUF;  // 以任意字节发送（不强制以每4个字节对其发送数据）
  sdmmc_host.slot = static_cast<int>(port_);
  sdmmc_host.max_freq_khz = freq_hz;

  uint8_t timeout_count = 0;
  while (1) {
    result = sdmmc_card_init(&sdmmc_host, sdio_handle_.get());
    if (result == ESP_OK) {
      break;
    }

    timeout_count++;
    if (timeout_count > kSdioBusInitTimeoutCount) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "sdmmc_card_init failed (error code: %#X)\n", result);
      return false;
    }
    DelayMs(100);
  }

  result = sdmmc_io_enable_int(sdio_handle_.get());
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_io_enable_int failed (error code: %#X)\n", result);
  }

  // sdmmc_card_print_info(stdout, sdio_handle_.get());

  freq_hz_ = freq_hz;

  return true;
}

bool HardwareSdio::WaitInterrupt(uint32_t timeout_ms) {
  esp_err_t result =
      sdmmc_io_wait_int(sdio_handle_.get(), pdMS_TO_TICKS(timeout_ms));
  if (result == ESP_ERR_TIMEOUT) {
    // LogMessage(LogLevel::kBus, __FILE__, __LINE__, "sdmmc_io_wait_int timeout
    // \n");
    return false;
  } else if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_io_wait_int failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareSdio::Read(
    uint32_t function, uint32_t write_c32, void* data, size_t byte) {
  esp_err_t result =
      sdmmc_io_read_bytes(sdio_handle_.get(), function, write_c32, data, byte);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_io_read_bytes failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareSdio::Read(uint32_t function, uint32_t write_c32, uint8_t* data) {
  esp_err_t result =
      sdmmc_io_read_byte(sdio_handle_.get(), function, write_c32, data);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_io_read_byte failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareSdio::ReadBlock(
    uint32_t function, uint32_t write_c32, void* data, size_t byte) {
  esp_err_t result =
      sdmmc_io_read_blocks(sdio_handle_.get(), function, write_c32, data, byte);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_io_read_blocks failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareSdio::Write(
    uint32_t function, uint32_t write_c32, const void* data, size_t byte) {
  esp_err_t result =
      sdmmc_io_write_bytes(sdio_handle_.get(), function, write_c32, data, byte);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_io_write_bytes failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareSdio::Write(uint32_t function, uint32_t write_c32, uint8_t data,
    uint8_t* read_d8_verify) {
  esp_err_t result = sdmmc_io_write_byte(
      sdio_handle_.get(), function, write_c32, data, read_d8_verify);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_io_write_byte failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareSdio::WriteBlock(
    uint32_t function, uint32_t write_c32, const void* data, size_t byte) {
  esp_err_t result = sdmmc_io_write_blocks(
      sdio_handle_.get(), function, write_c32, data, byte);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "sdmmc_io_write_blocks failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

#endif
}  // namespace cpp_bus_driver