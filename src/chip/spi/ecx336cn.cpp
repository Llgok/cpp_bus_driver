/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-04-22 17:36:48
 * @License: GPL 3.0
 */
#include "ecx336cn.h"

namespace cpp_bus_driver {
bool Ecx336cn::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    GpioWrite(rst_, 1);
    DelayMs(10);
    GpioWrite(rst_, 0);
    DelayMs(10);
    GpioWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipSpiGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  if (!InitSequence(
          kInitSequence640x400_60Hz_, sizeof(kInitSequence640x400_60Hz_))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  if (!SetPowerSaveMode(false)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetPowerSaveMode failed\n");
    return false;
  }

  return true;
}

bool Ecx336cn::SetPowerSaveMode(bool enable) {
  if (enable) {
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoPowerSaveMode), 0x0E)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else {
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoPowerSaveMode), 0x0F)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  return true;
}

}  // namespace cpp_bus_driver
