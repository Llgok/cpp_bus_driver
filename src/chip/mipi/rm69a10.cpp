/*
 * @Description: RM69A10 MIPI-DSI 显示面板驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-08-12 09:37:20
 * @License: GPL 3.0
 */
#include "chip/mipi/rm69a10.h"

namespace cpp_bus_driver {
bool Rm69a10::Init(float freq_mhz, float lane_bit_rate_mbps) {
  if (rst_ != kPinNotConnected) {
    bool result = true;
    result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);
    result &= GpioWrite(rst_, 0);
    DelayMs(10);
    result &= GpioWrite(rst_, 1);
    DelayMs(120);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  }

  if (!MipiChipBase::Init(freq_mhz, lane_bit_rate_mbps)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Init failed\n");
    return false;
  }

  auto buffer = GetChipId();
  if (buffer != kChipId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get rm69a10 chip id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get rm69a10 chip id success (id: %#X)\n", buffer);
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "InitSequence failed\n");
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

  if (!MipiChipBase::Deinit()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Deinit failed\n");
    result = false;
  }

  if (rst_ != kPinNotConnected) {
    result &= ResetGpio(rst_);
  }

  return result;
}

uint8_t Rm69a10::GetChipId() {
  uint8_t buffer = 0;

  if (!ReadCommand(static_cast<uint8_t>(DcsCommand::kRoChipId), &buffer, 1)) {
    return -1;
  }

  return buffer;
}

bool Rm69a10::SetSleep(bool enable) {
  if (!WriteCommand(enable ? static_cast<uint8_t>(DcsCommand::kWoSlpin)
                           : static_cast<uint8_t>(DcsCommand::kWoSlpout),
          nullptr, 0)) {
    return false;
  }

  DelayMs(120);

  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "RM69A10 display sleep state changed (sleep: %s)\n",
      enable ? "yes" : "no");

  return true;
}

bool Rm69a10::SetScreenOff(bool enable) {
  if (!WriteCommand(enable ? static_cast<uint8_t>(DcsCommand::kWoDispoff)
                           : static_cast<uint8_t>(DcsCommand::kWoDispon),
          nullptr, 0)) {
    return false;
  }

  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "RM69A10 display output state changed (screen off: %s)\n",
      enable ? "yes" : "no");

  return true;
}

bool Rm69a10::SetInversion(bool enable) {
  if (!WriteCommand(enable ? static_cast<uint8_t>(DcsCommand::kWoInvon)
                           : static_cast<uint8_t>(DcsCommand::kWoInvoff),
          nullptr, 0)) {
    return false;
  }

  return true;
}

bool Rm69a10::SetBrightness(uint8_t brightness) {
  const uint8_t command_data = brightness;

  if (!WriteCommand(
          static_cast<uint8_t>(DcsCommand::kWoWrdisbv), &command_data, 1)) {
    return false;
  }

  return true;
}

bool Rm69a10::SendColorStreamCoordinate(
    int x_start, int y_start, int x_end, int y_end, const void* data) {
  if (!bus_->Write(x_start, y_start, x_end, y_end, data)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "RM69A10 pixel stream write failed (x_start: %d, y_start: %d, x_end: "
        "%d, y_end: %d)\n",
        x_start, y_start, x_end, y_end);
    return false;
  }

  return true;
}
bool Rm69a10::ReadCommand(uint8_t command, uint8_t* data, size_t length) {
  if (bus_ != nullptr && bus_->Read(command, data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "RM69A10 command read failed (command: %#X)\n",
      static_cast<unsigned>(command));
  return false;
}

bool Rm69a10::WriteCommand(
    uint8_t command, const uint8_t* data, size_t length) {
  if (bus_ != nullptr && bus_->Write(command, data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "RM69A10 command write failed (command: %#X)\n",
      static_cast<unsigned>(command));
  return false;
}

}  // namespace cpp_bus_driver
