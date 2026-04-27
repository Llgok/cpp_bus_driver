/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-04-20 15:09:40
 * @License: GPL 3.0
 */
#include "s023msafjf10111e1.h"

namespace cpp_bus_driver {
bool S023msafjf10111e1::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetPinMode(rst_, PinMode::kOutput, PinStatus::kPullup);

    PinWrite(rst_, 1);
    DelayMs(10);
    PinWrite(rst_, 0);
    DelayMs(10);
    PinWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool S023msafjf10111e1::SetDataFormat(DataFormat format) {
  switch (format) {
    case DataFormat::kRgb888:
      if (!bus_->Write(
              static_cast<uint16_t>(Cmd::kRwInternalTestModeRegisterControl1),
              0x14)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }

      if (!bus_->Write(
              static_cast<uint16_t>(Cmd::kRwInternalTestModeRegisterControl2),
              0x40)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;
    case DataFormat::kInternalTestMode:
      if (!bus_->Write(
              static_cast<uint16_t>(Cmd::kRwInternalTestModeRegisterControl1),
              0x15)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }

      if (!bus_->Write(
              static_cast<uint16_t>(Cmd::kRwInternalTestModeRegisterControl2),
              0x80)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;

    default:
      break;
  }

  return true;
}

bool S023msafjf10111e1::SetInternalTestMode(InternalTestMode mode) {
  if (!bus_->Write(static_cast<uint16_t>(Cmd::kRwInternalTestMode),
          static_cast<uint8_t>(mode))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool S023msafjf10111e1::SetShowDirection(ShowDirection direction) {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint16_t>(Cmd::kRwHorizontalVerticalMirror1),
          &buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (!bus_->Read(static_cast<uint16_t>(Cmd::kRwHorizontalVerticalMirror2),
          &buffer[1])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  switch (direction) {
    case ShowDirection::kNormal:
      buffer[0] |= 0B00010000;
      buffer[1] &= 0B01111111;
      break;

    case ShowDirection::kHorizontalMirror:
      buffer[0] &= 0B11101111;
      buffer[1] &= 0B01111111;
      break;

    case ShowDirection::kVerticalMirror:
      buffer[0] |= 0B00010000;
      buffer[1] |= 0B10000000;
      break;

    case ShowDirection::kHorizontalVerticalMirror:
      buffer[0] &= 0B11101111;
      buffer[1] |= 0B10000000;
      break;

    default:
      LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "invalid show direction\n");
      return false;
  }

  if (!bus_->Write(static_cast<uint16_t>(Cmd::kRwHorizontalVerticalMirror1),
          buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(static_cast<uint16_t>(Cmd::kRwHorizontalVerticalMirror2),
          buffer[1])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool S023msafjf10111e1::SetBrightness(uint16_t value) {
  if (value > 511) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    value = 511;
  }

  if (!bus_->Write(
          static_cast<uint16_t>(Cmd::kRwDisplayBrightnessRegisterControl1),
          0x1C)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(
          static_cast<uint16_t>(Cmd::kRwDisplayBrightnessRegisterControl2),
          0x03)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(static_cast<uint16_t>(Cmd::kRwDisplayBrightness1),
          static_cast<uint8_t>(value))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(static_cast<uint16_t>(Cmd::kRwDisplayBrightness2),
          static_cast<uint8_t>(value >> 8))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
