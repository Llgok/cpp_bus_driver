/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-04-22 13:54:58
 * @License: GPL 3.0
 */
#include "hardware_i2c_1.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
bool HardwareI2c1::Init(uint32_t freq_hz, uint16_t address) {
  if ((bus_handle_ != nullptr) && (device_handle_ != nullptr)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "HardwareI2c1 has been initialized\n");
    return true;
  }

  if (freq_hz == static_cast<uint32_t>(CPP_BUS_DRIVER_DEFAULT_VALUE)) {
    freq_hz = CPP_BUS_DRIVER_DEFAULT_I2C_FREQ_HZ;
  }

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

  if (bus_handle_ == nullptr) {
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = port_,  // 选择I2C端口号，I2C_NUM_0表示使用I2C0
        .sda_io_num = static_cast<gpio_num_t>(sda_),
        .scl_io_num = static_cast<gpio_num_t>(scl_),
        .clk_source = I2C_CLK_SRC_DEFAULT,  // 使用默认的I2C时钟源
        .glitch_ignore_cnt = 7,  // 设置滤波器忽略的毛刺周期为7个I2C模块时钟周期
        .intr_priority = 0,      // 设置中断优先级
        .trans_queue_depth = 0,  // 设置传输队列深度
        .flags =
            {
                .enable_internal_pullup = 1,  // 启用内部上拉电阻
                .allow_pd = 0  // 不允许在睡眠模式下备份/恢复I2C寄存器
            },
    };

    esp_err_t result = i2c_new_master_bus(&bus_config, &bus_handle_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "i2c_new_master_bus failed (error code: %#X)\n", result);
      return false;
    }
  }

  if (address == static_cast<uint16_t>(CPP_BUS_DRIVER_DEFAULT_VALUE)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "address is null\n");
    return false;
  }

  if (device_handle_ == nullptr) {
    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = freq_hz,
        .scl_wait_us = 0,
        .flags =
            {
                .disable_ack_check = 1,
            },
    };

    esp_err_t result =
        i2c_master_bus_add_device(bus_handle_, &device_config, &device_handle_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "i2c_master_bus_add_device failed (error code: %#X)\n", result);
      return false;
    }
  }

  freq_hz_ = freq_hz;
  address_ = address;

  return true;
}

bool HardwareI2c1::Read(uint8_t* data, size_t length) {
  esp_err_t result = i2c_master_receive(
      device_handle_, data, length, CPP_BUS_DRIVER_DEFAULT_I2C_WAIT_TIMEOUT_MS);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_receive failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}
bool HardwareI2c1::Write(const uint8_t* data, size_t length) {
  esp_err_t result = i2c_master_transmit(
      device_handle_, data, length, CPP_BUS_DRIVER_DEFAULT_I2C_WAIT_TIMEOUT_MS);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_transmit failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}
bool HardwareI2c1::WriteRead(const uint8_t* write_data, size_t write_length,
    uint8_t* read_data, size_t read_length) {
  esp_err_t result =
      i2c_master_transmit_receive(device_handle_, write_data, write_length,
          read_data, read_length, CPP_BUS_DRIVER_DEFAULT_I2C_WAIT_TIMEOUT_MS);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2c_master_transmit_receive failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareI2c1::Probe(const uint16_t address) {
  esp_err_t result = i2c_master_probe(
      bus_handle_, address, CPP_BUS_DRIVER_DEFAULT_I2C_WAIT_TIMEOUT_MS);
  if (result != ESP_OK) {
    return false;
  }

  return true;
}

bool HardwareI2c1::set_bus_handle(i2c_master_bus_handle_t bus_handle) {
  if (bus_handle == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  bus_handle_ = bus_handle;

  return true;
}

i2c_master_bus_handle_t HardwareI2c1::bus_handle() {
  if (bus_handle_ == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return nullptr;
  }

  return bus_handle_;
}
#endif
}  // namespace cpp_bus_driver