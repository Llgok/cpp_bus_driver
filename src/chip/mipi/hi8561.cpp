/*
 * @Description: HI8561 MIPI-DSI 显示面板驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-09-02 16:15:50
 * @License: GPL 3.0
 */
#include "chip/mipi/hi8561.h"

namespace cpp_bus_driver {
bool Hi8561::Init(float freq_mhz, float lane_bit_rate_mbps) {
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
        "Get hi8561 chip id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get hi8561 chip id success (id: %#X)\n", buffer);
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

bool Hi8561::Deinit() {
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

uint16_t Hi8561::GetChipId() {
  uint8_t buffer[2] = {0};

  for (uint8_t i = 0; i < 2; i++) {
    if (!ReadCommand(static_cast<uint8_t>(DcsCommand::kRoChipIdStart) + i,
            &buffer[i], 1)) {
      return -1;
    }
  }

  return (static_cast<uint16_t>(buffer[0]) << 8) |
         static_cast<uint16_t>(buffer[1]);
}

bool Hi8561::SetSleep(bool enable) {
  if (!WriteCommand(enable ? static_cast<uint8_t>(DcsCommand::kWoSlpin)
                           : static_cast<uint8_t>(DcsCommand::kWoSlpout),
          nullptr, 0)) {
    return false;
  }

  DelayMs(120);

  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "HI8561 display sleep state changed (sleep: %s)\n",
      enable ? "yes" : "no");

  return true;
}

bool Hi8561::SetScreenOff(bool enable) {
  if (!WriteCommand(enable ? static_cast<uint8_t>(DcsCommand::kWoDispoff)
                           : static_cast<uint8_t>(DcsCommand::kWoDispon),
          nullptr, 0)) {
    return false;
  }

  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "HI8561 display output state changed (screen off: %s)\n",
      enable ? "yes" : "no");

  return true;
}

bool Hi8561::SetMirror(MirrorMode mode) {
  madctl_data_ &= 0B11111100;
  switch (mode) {
    case MirrorMode::kOff:
      break;

    case MirrorMode::kHorizontal:
      madctl_data_ |= 0B00000010;
      break;

    case MirrorMode::kVertical:
      madctl_data_ |= 0B00000001;
      break;

    case MirrorMode::kHorizontalVertical:
      madctl_data_ |= 0B00000011;
      break;

    default:
      break;
  }

  const uint8_t command_data = madctl_data_;

  if (!WriteCommand(
          static_cast<uint8_t>(DcsCommand::kWoMadctl), &command_data, 1)) {
    return false;
  }

  return true;
}

bool Hi8561::SetInversion(bool enable) {
  if (!WriteCommand(enable ? static_cast<uint8_t>(DcsCommand::kWoInvon)
                           : static_cast<uint8_t>(DcsCommand::kWoInvoff),
          nullptr, 0)) {
    return false;
  }

  return true;
}

bool Hi8561::SetBrightness(uint8_t brightness) {
  const uint8_t command_data = brightness;

  if (!WriteCommand(
          static_cast<uint8_t>(DcsCommand::kWoWrdisbv), &command_data, 1)) {
    return false;
  }

  return true;
}

bool Hi8561::SetColorOrder(ColorOrder order) {
  madctl_data_ =
      (madctl_data_ & 0xB11110111) | (static_cast<uint8_t>(order) << 3);

  const uint8_t command_data = madctl_data_;

  if (!WriteCommand(
          static_cast<uint8_t>(DcsCommand::kWoMadctl), &command_data, 1)) {
    return false;
  }

  return true;
}

bool Hi8561::SetCabcMode(CabcMode mode) {
  const uint8_t command_data = static_cast<uint8_t>(mode);

  if (!WriteCommand(
          static_cast<uint8_t>(DcsCommand::kWoWrcabc), &command_data, 1)) {
    return false;
  }

  return true;
}

bool Hi8561::SendColorStreamCoordinate(
    int x_start, int y_start, int x_end, int y_end, const void* data) {
  if (!bus_->Write(x_start, y_start, x_end, y_end, data)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 pixel stream write failed (x_start: %d, y_start: %d, x_end: "
        "%d, y_end: %d)\n",
        x_start, y_start, x_end, y_end);
    return false;
  }

  return true;
}

bool Hi8561::ReadCommand(uint8_t command, uint8_t* data, size_t length) {
  if (bus_ != nullptr && bus_->Read(command, data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "HI8561 command read failed (command: %#X)\n",
      static_cast<unsigned>(command));
  return false;
}

bool Hi8561::WriteCommand(uint8_t command, const uint8_t* data, size_t length) {
  if (bus_ != nullptr && bus_->Write(command, data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "HI8561 command write failed (command: %#X)\n",
      static_cast<unsigned>(command));
  return false;
}

}  // namespace cpp_bus_driver
