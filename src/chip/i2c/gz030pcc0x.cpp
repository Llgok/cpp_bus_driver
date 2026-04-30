/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-04-20 14:50:19
 * @License: GPL 3.0
 */
#include "gz030pcc0x.h"

namespace cpp_bus_driver {
bool Gz030pcc0x::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    GpioWrite(rst_, 1);
    DelayMs(10);
    GpioWrite(rst_, 0);
    DelayMs(10);
    GpioWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence) / sizeof(uint16_t))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  return true;
}

bool Gz030pcc0x::Deinit(bool delete_bus) {
  if (!ChipI2cGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kDisable, GpioStatus::kDisable);
  }

  return true;
}

float Gz030pcc0x::GetTemperatureCelsius() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint16_t>(Cmd::kRoTemperatureReading), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (0.51 * static_cast<float>(buffer)) - 63.0;
}

bool Gz030pcc0x::SetDataFormat(DataFormat format) {
  uint8_t buffer = 0;

  if (!bus_->Read(
          static_cast<uint16_t>(Cmd::kRwInternalTestModeInputDataFormat),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B11111000) | static_cast<uint8_t>(format);

  if (!bus_->Write(
          static_cast<uint16_t>(Cmd::kRwInternalTestModeInputDataFormat),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Gz030pcc0x::SetInternalTestMode(InternalTestMode mode) {
  uint8_t buffer = 0;

  if (!bus_->Read(
          static_cast<uint16_t>(Cmd::kRwInternalTestModeInputDataFormat),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B00011111) | static_cast<uint8_t>(mode);

  if (!bus_->Write(
          static_cast<uint16_t>(Cmd::kRwInternalTestModeInputDataFormat),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Gz030pcc0x::SetShowDirection(ShowDirection direction) {
  if (!bus_->Write(static_cast<uint16_t>(Cmd::kRwHorizontalVerticalMirror),
          static_cast<uint8_t>(direction))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Gz030pcc0x::SetBrightness(uint8_t value) {
  if (!bus_->Write(static_cast<uint16_t>(Cmd::kRwDisplayBrightness), value)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
