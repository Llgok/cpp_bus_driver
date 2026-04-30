/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-04-29 16:23:28
 * @License: GPL 3.0
 */
#include "hardware_i2c_2.h"

namespace cpp_bus_driver {
bool HardwareI2c2::Init(uint32_t freq_hz, uint16_t address) {
  if (freq_hz == static_cast<uint32_t>(CPP_BUS_DRIVER_DEFAULT_VALUE)) {
    freq_hz = CPP_BUS_DRIVER_DEFAULT_I2C_FREQ_HZ;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c2 config address: %#X\n", address);
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c2 config port_: %d\n", port_);
#endif
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c2 config sda_: %d\n", sda_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c2 config scl_: %d\n", scl_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2c2 config freq_hz: %d hz\n", freq_hz);

  if (address == static_cast<uint16_t>(CPP_BUS_DRIVER_DEFAULT_VALUE)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "address is null\n");
  }

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
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
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_param_config failed (error code: %#X)\n", result);
    return false;
  }

  result = i2c_driver_install(port_, I2C_MODE_MASTER, 0, 0, 0);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_driver_install failed (error code: %#X)\n", result);
  }

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF

  i2c_handle_->setPins(static_cast<uint8_t>(sda_), static_cast<uint8_t>(scl_));
  i2c_handle_->setClock(freq_hz);
  i2c_handle_->Init();
  
#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
  return false;
#endif

  freq_hz_ = freq_hz;
  address_ = address;

  return true;
}

bool HardwareI2c2::Deinit(bool delete_bus) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  esp_err_t result = i2c_driver_delete(port_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_driver_delete failed (error code: %#X)\n", result);
    return false;
  }

  return true;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF

  i2c_handle_->end();

  return true;
#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
  return false;
#endif
}

bool HardwareI2c2::Read(uint8_t* data, size_t length) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  esp_err_t result = i2c_master_read_from_device(port_, address_, data, length,
      pdMS_TO_TICKS(CPP_BUS_DRIVER_DEFAULT_I2C_WAIT_TIMEOUT_MS));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_read_from_device failed (error code: %#X)\n", result);
    return false;
  }

  return true;

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  if (!i2c_handle_->requestFrom(address_, length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "requestFrom failed\n");
    return false;
  }
  *data = i2c_handle_->Read();

  return true;
#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Read failed\n");
  return false;
#endif
}

bool HardwareI2c2::Write(const uint8_t* data, size_t length) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  esp_err_t result = i2c_master_write_to_device(port_, address_, data, length,
      pdMS_TO_TICKS(CPP_BUS_DRIVER_DEFAULT_I2C_WAIT_TIMEOUT_MS));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_write_to_device failed (error code: %#X)\n", result);
    return false;
  }

  return true;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  i2c_handle_->beginTransmission(address_);

  size_t result = i2c_handle_->write(data, length);
  if (result == 0) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  } else if (result != length) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "buffer is full\n");
  }

  result = i2c_handle_->endTransmission();
  switch (result) {
    case 1:
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "data too long\n");
      return false;
    case 2:
      LogMessage(
          LogLevel::kBus, __FILE__, __LINE__, "nack on transmit of address\n");
      return false;
    case 3:
      LogMessage(
          LogLevel::kBus, __FILE__, __LINE__, "nack on transmit of data\n");
      return false;
    case 4:
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "other error\n");
      return false;
    default:
      break;
  }

  return true;

#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
  return false;
#endif
}

bool HardwareI2c2::WriteRead(const uint8_t* write_data, size_t write_length,
    uint8_t* read_data, size_t read_length) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  esp_err_t result = i2c_master_write_read_device(port_, address_, write_data,
      write_length, read_data, read_length,
      pdMS_TO_TICKS(CPP_BUS_DRIVER_DEFAULT_I2C_WAIT_TIMEOUT_MS));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_write_read_device failed (error code: %#X)\n", result);
    return false;
  }

  return true;

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  i2c_handle_->beginTransmission(address_);

  size_t result = i2c_handle_->Write(write_data, write_length);
  if (result == 0) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  } else if (result != write_length) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "buffer is full\n");
  }

  result = i2c_handle_->endTransmission();
  switch (result) {
    case 1:
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "data too long\n");
      return false;
    case 2:
      LogMessage(
          LogLevel::kBus, __FILE__, __LINE__, "nack on transmit of address\n");
      return false;
    case 3:
      // LogMessage(LogLevel::kBus, __FILE__, __LINE__, "nack on transmit of
      // data\n"); return false;
    case 4:
      // LogMessage(LogLevel::kBus, __FILE__, __LINE__, "other error\n");
      // return false;
    default:
      break;
  }

  if (!i2c_handle_->requestFrom(address_, read_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "requestFrom failed\n");
    return false;
  }
  *read_data = i2c_handle_->Read();

  return true;

#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteRead failed\n");
  return false;
#endif
}

bool HardwareI2c2::Probe(const uint16_t address) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  uint8_t buffer = 0;
  esp_err_t result = i2c_master_read_from_device(port_, address, &buffer, 1,
      pdMS_TO_TICKS(CPP_BUS_DRIVER_DEFAULT_I2C_WAIT_TIMEOUT_MS));
  if (result != ESP_OK) {
    return false;
  }

  return true;

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF

  i2c_handle_->beginTransmission(address);
  uint8_t result = i2c_handle_->endTransmission();
  switch (result) {
    case 1:
      // LogMessage(LogLevel::kBus, __FILE__, __LINE__, "data too long\n");
      return false;
    case 2:
      // LogMessage(LogLevel::kBus, __FILE__, __LINE__, "nack on transmit of
      // address\n");
      return false;
    case 3:
      // LogMessage(LogLevel::kBus, __FILE__, __LINE__, "nack on transmit of
      // data\n");
      return false;
    case 4:
      // LogMessage(LogLevel::kBus, __FILE__, __LINE__, "other error\n");
      return false;
    default:
      break;
  }

  return true;

#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Probe failed\n");
  return false;
#endif
}

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
i2c_cmd_handle_t HardwareI2c2::CmdLinkCreate() { return i2c_cmd_link_create(); }

bool HardwareI2c2::StartTransmit(
    i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en) {
  esp_err_t result = i2c_master_start(cmd_handle);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_start failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }

  result = i2c_master_write_byte(cmd_handle, address_ << 1 | rw, ack_en);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_write_byte failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  return true;
}

bool HardwareI2c2::Write(
    i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en) {
  esp_err_t result = i2c_master_write_byte(cmd_handle, data, ack_en);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_write_byte failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  return true;
}

bool HardwareI2c2::Write(i2c_cmd_handle_t cmd_handle, const uint8_t* data,
    size_t data_len, bool ack_en) {
  esp_err_t result = i2c_master_write(cmd_handle, data, data_len, ack_en);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_write failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  return true;
}

bool HardwareI2c2::Read(i2c_cmd_handle_t cmd_handle, uint8_t* data,
    size_t data_len, i2c_ack_type_t ack) {
  esp_err_t result = i2c_master_read(cmd_handle, data, data_len, ack);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_read failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  return true;
}

bool HardwareI2c2::StopTransmit(i2c_cmd_handle_t cmd_handle) {
  esp_err_t result = i2c_master_stop(cmd_handle);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_stop failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }

  result = i2c_master_cmd_begin(port_, cmd_handle,
      pdMS_TO_TICKS(CPP_BUS_DRIVER_DEFAULT_I2C_WAIT_TIMEOUT_MS));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_cmd_begin failed (error code: %#X)\n", result);
    i2c_cmd_link_delete(cmd_handle);
    return false;
  }
  i2c_cmd_link_delete(cmd_handle);
  return true;
}
#endif

}  // namespace cpp_bus_driver
