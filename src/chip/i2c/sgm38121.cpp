/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-04-20 15:10:33
 * @License: GPL 3.0
 */
#include "sgm38121.h"

namespace cpp_bus_driver {
bool Sgm38121::Init(int32_t freq_hz) {
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

  auto buffer = GetDeviceId();
  if (buffer != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get sgm38121 id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get sgm38121 id success (id: %#X)\n", buffer);
  }

  return true;
}

uint8_t Sgm38121::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Sgm38121::SetOutputVoltage(Channel channel, uint16_t voltage) {
  uint8_t buffer = 0;

  switch (channel) {
    case Channel::kDvdd1:
      if (voltage < 528) {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
        voltage = 528;
      } else if (voltage > 1504) {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
        voltage = 1504;
      }
      buffer = (voltage - 504) / 8;
      if (!bus_->Write(
              static_cast<uint8_t>(Cmd::kRwDvdd1OutputVoltageLevel), buffer)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;
    case Channel::kDvdd2:
      if (voltage < 528) {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
        voltage = 528;
      } else if (voltage > 1504) {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
        voltage = 1504;
      }
      buffer = (voltage - 504) / 8;
      if (!bus_->Write(
              static_cast<uint8_t>(Cmd::kRwDvdd2OutputVoltageLevel), buffer)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;
    case Channel::kAvdd1:
      if (voltage < 1504) {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
        voltage = 1504;
      } else if (voltage > 3424) {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
        voltage = 3424;
      }
      buffer = (voltage - 1384) / 8;
      if (!bus_->Write(
              static_cast<uint8_t>(Cmd::kRwAvdd1OutputVoltageLevel), buffer)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;
    case Channel::kAvdd2:
      if (voltage < 1504) {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
        voltage = 1504;
      } else if (voltage > 3424) {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
        voltage = 3424;
      }
      buffer = (voltage - 1384) / 8;
      if (!bus_->Write(
              static_cast<uint8_t>(Cmd::kRwAvdd2OutputVoltageLevel), buffer)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;

    default:
      break;
  }

  return true;
}

bool Sgm38121::SetChannelStatus(Channel channel, Status status) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwEnableControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  switch (channel) {
    case Channel::kDvdd1:
      buffer = (buffer & 0B11111110) | static_cast<uint8_t>(status);
      break;
    case Channel::kDvdd2:
      buffer = (buffer & 0B11111101) | (static_cast<uint8_t>(status) << 1);
      break;
    case Channel::kAvdd1:
      buffer = (buffer & 0B11111011) | (static_cast<uint8_t>(status) << 2);
      break;
    case Channel::kAvdd2:
      buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(status) << 3);
      break;

    default:
      break;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwEnableControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}
}  // namespace cpp_bus_driver
