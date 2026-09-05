/*
 * @Description: 跨平台硬件 SPI 总线驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-09-05 14:57:23
 * @License: GPL 3.0
 */
#include "bus/spi/hardware_spi.h"

#include <algorithm>
#include <array>
#include <new>

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52

namespace cpp_bus_driver {

bool HardwareSpi::Init(int32_t freq_hz, int32_t cs) {
  if (freq_hz <= 0) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Invalid bus frequency\n");
    return false;
  }
  if (spi_handle_ != nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HardwareSpi has already been initialized\n");
    return false;
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
      "HardwareSpi config port_ address: %#X\n", port_);

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config mode_: %d\n", mode_);

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config bit_order_: %d\n", bit_order_);

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareSpi config freq_hz: %d hz\n", freq_hz);

  spi_handle_.reset(
      new (std::nothrow) SPIClass(port_, static_cast<uint8_t>(miso_),
          static_cast<uint8_t>(sclk_), static_cast<uint8_t>(mosi_)));
  if (spi_handle_ == nullptr) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "Allocate SPI handle failed\n");
    return false;
  }
  bool result = true;
  if (cs != kPinNotConnected) {
    result &= SetGpioMode(cs, GpioMode::kOutput);
    result &= GpioWrite(cs, 1);
  }
  if (!result) {
    if (cs != kPinNotConnected) {
      static_cast<void>(ResetGpio(cs));
    }
    spi_handle_.reset();
    return false;
  }
  spi_settings_ = SPISettings(freq_hz, bit_order_, mode_);

  spi_handle_->begin();

  cs_ = cs;

  return true;
}

bool HardwareSpi::Deinit(bool delete_bus) {
  bool result = true;
  if (spi_handle_ != nullptr) {
    spi_handle_->end();
    spi_handle_.reset();
  }
  if (cs_ != kPinNotConnected) {
    result &= ResetGpio(cs_);
  }
  cs_ = kPinNotConnected;
  return result;
}

bool HardwareSpi::Write(const void* data, size_t byte) {
  if (spi_handle_ == nullptr) {
    return false;
  }
  bool result = true;
  spi_handle_->beginTransaction(spi_settings_);
  if (cs_ != kPinNotConnected) {
    result &= GpioWrite(cs_, 0);
  }
  if (result) {
    spi_handle_->transfer(const_cast<void*>(data), byte);
  }
  if (cs_ != kPinNotConnected) {
    result &= GpioWrite(cs_, 1);
  }
  spi_handle_->endTransaction();

  return result;
}

bool HardwareSpi::Read(void* data, size_t byte) {
  if (spi_handle_ == nullptr || (data == nullptr && byte != 0)) {
    return false;
  }
  if (byte == 0) {
    return true;
  }
  // 只保存一小块零填充数据；所有分块共享同一次事务和 CS 有效区间。
  std::array<uint8_t, 128> buffer{};

  bool result = true;
  spi_handle_->beginTransaction(spi_settings_);
  if (cs_ != kPinNotConnected) {
    result &= GpioWrite(cs_, 0);
  }
  if (result) {
    auto* output = static_cast<uint8_t*>(data);
    size_t offset = 0;
    while (offset < byte) {
      const size_t length = std::min(buffer.size(), byte - offset);
      buffer.fill(0);
      spi_handle_->transfer(buffer.data(), output + offset, length);
      offset += length;
    }
  }
  if (cs_ != kPinNotConnected) {
    result &= GpioWrite(cs_, 1);
  }
  spi_handle_->endTransaction();

  return result;
}

bool HardwareSpi::WriteRead(
    const void* write_data, void* read_data, size_t data_byte) {
  if (spi_handle_ == nullptr) {
    return false;
  }
  bool result = true;
  spi_handle_->beginTransaction(spi_settings_);
  if (cs_ != kPinNotConnected) {
    result &= GpioWrite(cs_, 0);
  }
  if (result) {
    spi_handle_->transfer(write_data, read_data, data_byte);
  }
  if (cs_ != kPinNotConnected) {
    result &= GpioWrite(cs_, 1);
  }
  spi_handle_->endTransaction();

  return result;
}
}  // namespace cpp_bus_driver

#endif
