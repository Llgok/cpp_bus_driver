/*
 * @Description: GZ030PCC0X 显示面板辅助控制驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-09-02 16:15:31
 * @License: GPL 3.0
 */
#include "chip/i2c/gz030pcc0x.h"

namespace cpp_bus_driver {
bool Gz030pcc0x::Init(int32_t freq_hz) {
  if (rst_ != kPinNotConnected) {
    bool result = true;
    result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);
    result &= GpioWrite(rst_, 0);
    DelayMs(10);
    result &= GpioWrite(rst_, 1);
    DelayMs(10);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  }

  if (!I2cChipBase::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Init failed\n");
    return false;
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence) / sizeof(uint16_t))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "InitSequence failed\n");
    return false;
  }

  return true;
}

bool Gz030pcc0x::Deinit(bool delete_bus) {
  bool result = true;

  if (!I2cChipBase::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Deinit failed\n");
    result = false;
  }

  if (rst_ != kPinNotConnected) {
    result &= ResetGpio(rst_);
  }

  return result;
}

float Gz030pcc0x::GetTemperatureCelsius() {
  uint8_t buffer = 0;

  if (!ReadRegister(
          static_cast<uint16_t>(Register::kRoTemperatureReading), &buffer)) {
    return -1;
  }

  return (0.51 * static_cast<float>(buffer)) - 63.0;
}

bool Gz030pcc0x::SetDataFormat(DataFormat format) {
  uint8_t buffer = 0;

  if (!ReadRegister(
          static_cast<uint16_t>(Register::kRwInternalTestModeInputDataFormat),
          &buffer)) {
    return false;
  }

  buffer = (buffer & 0B11111000) | static_cast<uint8_t>(format);

  if (!WriteRegister(
          static_cast<uint16_t>(Register::kRwInternalTestModeInputDataFormat),
          buffer)) {
    return false;
  }

  return true;
}

bool Gz030pcc0x::SetInternalTestMode(InternalTestMode mode) {
  uint8_t buffer = 0;

  if (!ReadRegister(
          static_cast<uint16_t>(Register::kRwInternalTestModeInputDataFormat),
          &buffer)) {
    return false;
  }

  buffer = (buffer & 0B00011111) | static_cast<uint8_t>(mode);

  if (!WriteRegister(
          static_cast<uint16_t>(Register::kRwInternalTestModeInputDataFormat),
          buffer)) {
    return false;
  }

  return true;
}

bool Gz030pcc0x::SetShowDirection(ShowDirection direction) {
  if (!WriteRegister(
          static_cast<uint16_t>(Register::kRwHorizontalVerticalMirror),
          static_cast<uint8_t>(direction))) {
    return false;
  }

  return true;
}

bool Gz030pcc0x::SetBrightness(uint8_t value) {
  if (!WriteRegister(
          static_cast<uint16_t>(Register::kRwDisplayBrightness), value)) {
    return false;
  }

  return true;
}

bool Gz030pcc0x::ReadRegister(uint16_t reg, uint8_t* data, size_t length) {
  const uint8_t register_packet[] = {
      static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg)};

  if (bus_ != nullptr &&
      bus_->WriteRead(register_packet, sizeof(register_packet), data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "GZ030PCC0X register read failed (register: %#X)\n",
      static_cast<unsigned>(reg));
  return false;
}

bool Gz030pcc0x::WriteRegister(uint16_t reg, uint8_t value) {
  const uint8_t register_packet[] = {
      static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg), value};

  if (bus_ != nullptr &&
      bus_->Write(register_packet, sizeof(register_packet))) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "GZ030PCC0X register write failed (register: %#X)\n",
      static_cast<unsigned>(reg));
  return false;
}
}  // namespace cpp_bus_driver
