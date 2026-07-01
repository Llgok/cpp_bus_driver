/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-04-20 15:37:22
 * @License: GPL 3.0
 */
#include "rm69a10.h"

namespace cpp_bus_driver {
bool Rm69a10::Init(float freq_mhz, float lane_bit_rate_mbps) {
  if (rst_ != kDefaultValue) {
    bool result = true;
    result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);
    result &= GpioWrite(rst_, 1);
    DelayMs(5);
    result &= GpioWrite(rst_, 0);
    DelayMs(10);
    result &= GpioWrite(rst_, 1);
    DelayMs(120);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  }

  if (!ChipMipiGuide::Init(freq_mhz, lane_bit_rate_mbps)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get rm69a10 id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get rm69a10 id success (id: %#X)\n", buffer);
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  if (!bus_->StartTransmit()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "StartTransmit failed\n");
    return false;
  }

  return true;
}

bool Rm69a10::Deinit() {
  bool result = true;

  if (!ChipMipiGuide::Deinit()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
    result = false;
  }

  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }

  return result;
}

uint8_t Rm69a10::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Rm69a10::SetSleep(bool enable) {
  if (!bus_->Write(enable ? static_cast<uint8_t>(Cmd::kWoSlpin)
                          : static_cast<uint8_t>(Cmd::kWoSlpout))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  DelayMs(120);

  return true;
}

bool Rm69a10::SetScreenOff(bool enable) {
  if (!bus_->Write(enable ? static_cast<uint8_t>(Cmd::kWoDispoff)
                          : static_cast<uint8_t>(Cmd::kWoDispon))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Rm69a10::SetInversion(bool enable) {
  if (!bus_->Write(enable ? static_cast<uint8_t>(Cmd::kWoInvon)
                          : static_cast<uint8_t>(Cmd::kWoInvoff))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Rm69a10::SetBrightness(uint8_t brightness) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWrdisbv), brightness)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Rm69a10::SendColorStreamCoordinate(
    int x_start, int y_start, int x_end, int y_end, const void* data) {
  if (!bus_->Write(x_start, y_start, x_end, y_end, data)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}
}  // namespace cpp_bus_driver
