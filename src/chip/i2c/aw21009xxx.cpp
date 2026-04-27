/*
 * @Description: None
 * @Author: None
 * @Date: 2025-09-24 10:47:30
 * @LastEditTime: 2026-04-24 09:24:41
 * @License: GPL 3.0
 */
#include "aw21009xxx.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
constexpr const uint8_t Aw21009xxx::kInitSequence[];
#endif

bool Aw21009xxx::Init(int32_t freq_hz) {
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

  auto buffer = GetDeviceId();
  if (buffer != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get aw21009xxx id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get aw21009xxx id success (id: %#X)\n", buffer);
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  return true;
}

uint8_t Aw21009xxx::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Aw21009xxx::SetAutoPowerSave(bool enable) {
  uint8_t buffer = 0;

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwGlobalControlRegister), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B01111111) | (enable << 7);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwGlobalControlRegister), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw21009xxx::SetChipEnable(bool enable) {
  uint8_t buffer = 0;

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwGlobalControlRegister), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B11111110) | enable;

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwGlobalControlRegister), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw21009xxx::SetBrightness(LedChannel channel, uint16_t value) {
  if (value > 4095) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    value = 4095;
  }

  if (channel == LedChannel::kAll) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(LedChannel::kAll); i++) {
      uint8_t buffer_address_lsb =
          static_cast<uint8_t>(Cmd::kRwBrightnessControlRegisterStart) +
          (i * 2);
      uint8_t buffer_address_msb = buffer_address_lsb + 1;

      if (!bus_->Write(buffer_address_lsb, value)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }

      uint8_t buffer_msb_value = (value >> 8) & 0x0F;
      if (!bus_->Write(buffer_address_msb, buffer_msb_value)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
    }
  } else {
    uint8_t buffer_address_lsb =
        static_cast<uint8_t>(Cmd::kRwBrightnessControlRegisterStart) +
        (static_cast<uint8_t>(channel) * 2);
    uint8_t buffer_address_msb = buffer_address_lsb + 1;

    if (!bus_->Write(buffer_address_lsb, value)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }

    uint8_t buffer_msb_value = (value >> 8) & 0x0F;
    if (!bus_->Write(buffer_address_msb, buffer_msb_value)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoUpdateRegister),
          static_cast<uint8_t>(0))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw21009xxx::SetCurrentLimit(LedChannel channel, uint8_t value) {
  if (channel == LedChannel::kAll) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(LedChannel::kAll); i++) {
      if (!bus_->Write(
              static_cast<uint8_t>(
                  static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) + i),
              value)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
    }
  } else {
    if (!bus_->Write(static_cast<uint8_t>(
                         static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) +
                         static_cast<uint8_t>(channel)),
            value)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoUpdateRegister),
          static_cast<uint8_t>(0))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw21009xxx::SetGlobalCurrentLimit(uint8_t value) {
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwGlobalCurrentControlRegister), value)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
