/*
 * @Description: ECX336CN SPI 显示控制器驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-04-22 17:36:48
 * @License: GPL 3.0
 */
#include "ecx336cn.h"

namespace cpp_bus_driver {
bool Ecx336cn::Init(int32_t freq_hz) {
  if (rst_ != kDefaultValue) {
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

  if (!ChipSpiGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  if (!InitSequence(
          kInitSequence640x400_60Hz_, sizeof(kInitSequence640x400_60Hz_))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  if (!SetPowerSaveMode(false)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetPowerSaveMode failed\n");
    return false;
  }

  return true;
}

bool Ecx336cn::Deinit(bool delete_bus) {
  bool result = true;

  if (!ChipSpiGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
    result = false;
  }

  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }

  return result;
}

bool Ecx336cn::SetPowerSaveMode(bool enable) {
  if (enable) {
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoPowerSaveMode), 0x0E)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else {
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoPowerSaveMode), 0x0F)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  return true;
}

}  // namespace cpp_bus_driver
