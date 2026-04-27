/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2026-04-21 11:40:10
 * @License: GPL 3.0
 */
#include "xl95x5.h"

namespace cpp_bus_driver {
bool Xl95x5::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    Tool::SetPinMode(rst_, PinMode::kOutput, PinStatus::kPullup);

    Tool::PinWrite(rst_, 1);
    DelayMs(10);
    Tool::PinWrite(rst_, 0);
    DelayMs(10);
    Tool::PinWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer == static_cast<uint8_t>(CPP_BUS_DRIVER_DEFAULT_VALUE)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get xl95x5 id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get xl95x5 id success (id: %#X)\n", buffer);
  }

  return true;
}

uint8_t Xl95x5::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Xl95x5::SetPinMode(Pin pin, Mode mode) {
  uint8_t buffer = 0;

  if (pin == Pin::kIoPort0) {
    if (mode == Mode::kOutput) {
      buffer = 0B00000000;
    } else {
      buffer = 0B11111111;
    }
    if (!bus_->Write(
            static_cast<uint8_t>(Cmd::kRwConfigurationPort0), buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else if (pin == Pin::kIoPort1) {
    if (mode == Mode::kOutput) {
      buffer = 0B00000000;
    } else {
      buffer = 0B11111111;
    }
    if (!bus_->Write(
            static_cast<uint8_t>(Cmd::kRwConfigurationPort1), buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else if (static_cast<uint8_t>(pin) > 7) {
    if (!bus_->Read(
            static_cast<uint8_t>(Cmd::kRwConfigurationPort1), &buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
    if (mode == Mode::kOutput)  // 写0输出，写1输入
    {
      buffer = buffer & (~(1 << (static_cast<uint8_t>(pin) - 10)));
    } else {
      buffer = buffer | (1 << (static_cast<uint8_t>(pin) - 10));
    }
    if (!bus_->Write(
            static_cast<uint8_t>(Cmd::kRwConfigurationPort1), buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else {
    if (!bus_->Read(
            static_cast<uint8_t>(Cmd::kRwConfigurationPort0), &buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
    if (mode == Mode::kOutput)  // 写0输出，写1输入
    {
      buffer = buffer & (~(1 << static_cast<uint8_t>(pin)));
    } else {
      buffer = buffer | (1 << static_cast<uint8_t>(pin));
    }
    if (!bus_->Write(
            static_cast<uint8_t>(Cmd::kRwConfigurationPort0), buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  return true;
}

bool Xl95x5::PinWrite(Pin pin, Value value) {
  uint8_t buffer = 0;

  if (pin == Pin::kIoPort0) {
    if (value == Value::kLow)  // 写0为低电平，写1为高电平
    {
      buffer = 0B00000000;
    } else {
      buffer = 0B11111111;
    }
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwOutputPort0), buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else if (pin == Pin::kIoPort1) {
    if (value == Value::kLow)  // 写0为低电平，写1为高电平
    {
      buffer = 0B00000000;
    } else {
      buffer = 0B11111111;
    }
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwOutputPort1), buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }
  if (static_cast<uint8_t>(pin) > 7) {
    if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwOutputPort1), &buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
    if (value == Value::kLow)  // 写0为低电平，写1为高电平
    {
      buffer = buffer & (~(1 << (static_cast<uint8_t>(pin) - 10)));
    } else {
      buffer = buffer | (1 << (static_cast<uint8_t>(pin) - 10));
    }

    if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwOutputPort1), buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else {
    if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwOutputPort0), &buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
    if (value == Value::kLow)  // 写0为低电平，写1为高电平
    {
      buffer = buffer & (~(1 << static_cast<uint8_t>(pin)));
    } else {
      buffer = buffer | (1 << static_cast<uint8_t>(pin));
    }
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwOutputPort0), buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  return true;
}

bool Xl95x5::PinRead(Pin pin) {
  uint8_t buffer = 0;

  // 写0为低电平，写1为高电平
  if (static_cast<uint8_t>(pin) > 7) {
    if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoInputPort1), &buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return -1;
    }
    buffer = (buffer >> (static_cast<uint8_t>(pin) - 10)) & 0B00000001;
  } else {
    if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoInputPort0), &buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return -1;
    }
    buffer = (buffer >> static_cast<uint8_t>(pin)) & 0B00000001;
  }

  return buffer;
}

bool Xl95x5::ClearIrqFlag() {
  uint8_t buffer = 0;

  for (uint8_t i = 0; i < 2; i++) {
    if (!bus_->Read(
            static_cast<uint8_t>(static_cast<uint8_t>(Cmd::kRoInputPort0) + i),
            &buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
  }

  return true;
}
}  // namespace cpp_bus_driver
