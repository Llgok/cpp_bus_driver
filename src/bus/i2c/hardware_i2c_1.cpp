/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-07-01 13:50:41
 * @License: GPL 3.0
 */
#include "hardware_i2c_1.h"

namespace cpp_bus_driver {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
bool HardwareI2c1::InitBus(uint32_t freq_hz) {
  if (freq_hz == static_cast<uint32_t>(kDefaultValue)) {
    freq_hz = kDefaultI2cFreqHz;
  }

  if (shared_bus_provider_ != nullptr) {
    if (bus_handle_ != nullptr) {
      return true;
    }
    if (!shared_bus_provider_->InitBus(freq_hz)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "Init shared i2c bus failed\n");
      return false;
    }

    bus_handle_ = shared_bus_provider_->bus_handle();
    freq_hz_ = freq_hz;
    delete_bus_on_deinit_ = false;
    if (bus_handle_ != nullptr) {
      bus_init_state_.store(BusInitState::kReady);
    }

    return bus_handle_ != nullptr;
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
            "Wait i2c bus init timeout\n");
        return false;
      }
      DelayMs(1);
    }

    return bus_init_state_.load() == BusInitState::kReady;
  }

  const i2c_master_bus_config_t bus_config = {
      .i2c_port = port_,
      .sda_io_num = static_cast<gpio_num_t>(sda_),
      .scl_io_num = static_cast<gpio_num_t>(scl_),
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .intr_priority = 0,
      .trans_queue_depth = 0,
      .flags =
          {
              .enable_internal_pullup = 1,
              .allow_pd = 0,
          },
  };

  esp_err_t result = i2c_new_master_bus(&bus_config, &bus_handle_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_new_master_bus failed (error code: %#X)\n", result);
    bus_init_state_.store(BusInitState::kNotStarted);
    return false;
  }

  freq_hz_ = freq_hz;
  delete_bus_on_deinit_ = true;
  bus_init_state_.store(BusInitState::kReady);

  return true;
}

bool HardwareI2c1::Init(uint32_t freq_hz, uint16_t address) {
  if (freq_hz == static_cast<uint32_t>(kDefaultValue)) {
    freq_hz = kDefaultI2cFreqHz;
  }
  const bool had_bus = bus_handle_ != nullptr;

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c1 config address: %#X\n", address);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c1 config port_: %d\n", port_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c1 config sda_: %d\n", sda_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c1 config scl_: %d\n", scl_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c1 config freq_hz: %d hz\n", freq_hz);

  if (!InitBus(freq_hz)) {
    return false;
  }
  const bool created_bus = !had_bus && delete_bus_on_deinit_;

  if (address == static_cast<uint16_t>(kDefaultValue)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "address is null\n");
    freq_hz_ = freq_hz;
    address_ = address;
    return true;
  }

  if (device_handle_ == nullptr) {
    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = freq_hz,
        .scl_wait_us = 0,
        .flags =
            {
                .disable_ack_check = 0,
            },
    };

    esp_err_t result = i2c_master_bus_add_device(
        bus_handle_, &device_config, &device_handle_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2c_master_bus_add_device failed (error code: %#X)\n", result);
      Deinit(created_bus);
      return false;
    }
  }

  freq_hz_ = freq_hz;
  address_ = address;

  return true;
}

bool HardwareI2c1::Deinit(bool delete_bus) {
  bool result = true;

  if (device_handle_ != nullptr) {
    esp_err_t ret = i2c_master_bus_rm_device(device_handle_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2c_master_bus_rm_device failed (error code: %#X)\n", ret);
      result = false;
    } else {
      device_handle_ = nullptr;
    }
  }

  if (delete_bus && bus_handle_ != nullptr) {
    if (!delete_bus_on_deinit_) {
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "Skip deleting external i2c bus\n");
      return result;
    }

    esp_err_t ret = i2c_del_master_bus(bus_handle_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2c_del_master_bus failed (error code: %#X)\n", ret);
      result = false;
    } else {
      bus_handle_ = nullptr;
      delete_bus_on_deinit_ = false;
      bus_init_state_.store(BusInitState::kNotStarted);
      if (sda_ != kDefaultValue) {
        result &= ResetGpio(sda_);
      }
      if (scl_ != kDefaultValue) {
        result &= ResetGpio(scl_);
      }
    }
  }

  return result;
}

bool HardwareI2c1::Read(uint8_t* data, size_t length) {
  esp_err_t result = i2c_master_receive(
      device_handle_, data, length,
      kDefaultI2cWaitTimeoutMs);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_receive failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareI2c1::Write(const uint8_t* data, size_t length) {
  esp_err_t result = i2c_master_transmit(
      device_handle_, data, length,
      kDefaultI2cWaitTimeoutMs);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_transmit failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareI2c1::WriteRead(const uint8_t* write_data, size_t write_length,
    uint8_t* read_data, size_t read_length) {
  esp_err_t result =
      i2c_master_transmit_receive(device_handle_, write_data, write_length,
          read_data, read_length, kDefaultI2cWaitTimeoutMs);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_transmit_receive failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareI2c1::Probe(const uint16_t address) {
  esp_err_t result = i2c_master_probe(
      bus_handle_, address, kDefaultI2cWaitTimeoutMs);
  if (result != ESP_OK) {
    return false;
  }

  return true;
}

bool HardwareI2c1::set_bus_handle(i2c_master_bus_handle_t bus_handle) {
  if (bus_handle == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (device_handle_ != nullptr ||
      (bus_handle_ != nullptr && delete_bus_on_deinit_)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HardwareI2c1 has been initialized\n");
    return false;
  }

  bus_handle_ = bus_handle;
  shared_bus_provider_.reset();
  delete_bus_on_deinit_ = false;
  bus_init_state_.store(BusInitState::kReady);

  return true;
}

i2c_master_bus_handle_t HardwareI2c1::bus_handle() {
  if (bus_handle_ == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return nullptr;
  }

  return bus_handle_;
}
#endif
}  // namespace cpp_bus_driver
