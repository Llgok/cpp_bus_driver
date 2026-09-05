/*
 * @Description: ESP-IDF 旧版硬件 I2C 总线兼容驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-09-05 11:10:00
 * @LastEditTime: 2026-09-05 14:57:40
 * @License: GPL 3.0
 */
#include <limits>

#include "bus/i2c/hardware_i2c.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

// 旧版实现必须独立编译，避免使用 HardwareI2c 时连带链接旧版驱动，
// 触发 ESP-IDF 新旧 I2C 驱动的启动冲突检查。
namespace cpp_bus_driver {
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
bool LegacyHardwareI2c::Init(uint32_t freq_hz, uint16_t address) {
  if (freq_hz == 0 ||
      freq_hz > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Invalid I2C frequency\n");
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "LegacyHardwareI2c config address: %#X\n", address);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "LegacyHardwareI2c config port_: %d\n", port_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "LegacyHardwareI2c config sda_: %d\n", sda_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "LegacyHardwareI2c config scl_: %d\n", scl_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "LegacyHardwareI2c config freq_hz: %d hz\n", freq_hz);

  if (address == kNoDeviceAddress) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "address is null\n");
  }

  const i2c_config_t i2c_config = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = static_cast<gpio_num_t>(sda_),
      .scl_io_num = static_cast<gpio_num_t>(scl_),
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master =
          {
              .clk_speed = freq_hz,
          },
      .clk_flags = 0,
  };

  esp_err_t result = i2c_param_config(port_, &i2c_config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_param_config failed (error code: %#X)\n", result);
    return false;
  }

  result = i2c_driver_install(port_, I2C_MODE_MASTER, 0, 0, 0);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_driver_install failed (error code: %#X)\n", result);
    return false;
  }

  address_ = address;

  return true;
}

bool LegacyHardwareI2c::Deinit(bool delete_bus) {
  esp_err_t result = i2c_driver_delete(port_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_driver_delete failed (error code: %#X)\n", result);
    return false;
  }

  bool gpio_result = true;
  if (sda_ != kPinNotConnected) {
    gpio_result &= ResetGpio(sda_);
  }
  if (scl_ != kPinNotConnected) {
    gpio_result &= ResetGpio(scl_);
  }

  return gpio_result;
}

bool LegacyHardwareI2c::Read(uint8_t* data, size_t length) {
  esp_err_t result = i2c_master_read_from_device(
      port_, address_, data, length, pdMS_TO_TICKS(kDefaultWaitTimeoutMs));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_read_from_device failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool LegacyHardwareI2c::Write(const uint8_t* data, size_t length) {
  esp_err_t result = i2c_master_write_to_device(
      port_, address_, data, length, pdMS_TO_TICKS(kDefaultWaitTimeoutMs));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_write_to_device failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool LegacyHardwareI2c::WriteRead(const uint8_t* write_data,
    size_t write_length, uint8_t* read_data, size_t read_length) {
  esp_err_t result =
      i2c_master_write_read_device(port_, address_, write_data, write_length,
          read_data, read_length, pdMS_TO_TICKS(kDefaultWaitTimeoutMs));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_write_read_device failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool LegacyHardwareI2c::Probe(const uint16_t address) {
  uint8_t buffer = 0;
  esp_err_t result = i2c_master_read_from_device(
      port_, address, &buffer, 1, pdMS_TO_TICKS(kDefaultWaitTimeoutMs));
  if (result != ESP_OK) {
    return false;
  }

  return true;
}

i2c_cmd_handle_t LegacyHardwareI2c::CreateCommandLink() {
  return i2c_cmd_link_create();
}

bool LegacyHardwareI2c::StartTransmit(
    i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en) {
  esp_err_t result = i2c_master_start(cmd_handle);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_start failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }

  result = i2c_master_write_byte(cmd_handle, address_ << 1 | rw, ack_en);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_write_byte failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  return true;
}

bool LegacyHardwareI2c::Write(
    i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en) {
  esp_err_t result = i2c_master_write_byte(cmd_handle, data, ack_en);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_write_byte failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  return true;
}

bool LegacyHardwareI2c::Write(i2c_cmd_handle_t cmd_handle, const uint8_t* data,
    size_t data_len, bool ack_en) {
  esp_err_t result = i2c_master_write(cmd_handle, data, data_len, ack_en);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_write failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  return true;
}

bool LegacyHardwareI2c::Read(i2c_cmd_handle_t cmd_handle, uint8_t* data,
    size_t data_len, i2c_ack_type_t ack) {
  esp_err_t result = i2c_master_read(cmd_handle, data, data_len, ack);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_read failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  return true;
}

bool LegacyHardwareI2c::StopTransmit(i2c_cmd_handle_t cmd_handle) {
  esp_err_t result = i2c_master_stop(cmd_handle);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_stop failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }

  result = i2c_master_cmd_begin(
      port_, cmd_handle, pdMS_TO_TICKS(kDefaultWaitTimeoutMs));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2c_master_cmd_begin failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  i2c_cmd_link_delete(cmd_handle);
  return true;
}
#endif

}  // namespace cpp_bus_driver

#endif
