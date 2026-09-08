/*
 * @Description: XL95x5 GPIO 扩展芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2026-09-05 14:56:58
 * @License: GPL 3.0
 */
#include "chip/i2c/xl95x5.h"

namespace cpp_bus_driver {
bool Xl95x5::Init(int32_t freq_hz) {
  if (rst_ != kPinNotConnected) {
    bool result = true;
    result &=
        PlatformHal::SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);
    result &= PlatformHal::GpioWrite(rst_, 0);
    DelayMs(10);
    result &= PlatformHal::GpioWrite(rst_, 1);
    DelayMs(10);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  }

  if (!I2cChipBase::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Init failed\n");
    return false;
  }

  auto buffer = GetChipId();
  if (buffer == kInvalidChipId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get xl95x5 chip id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get xl95x5 chip id success (id: %#X)\n", buffer);
  }

  return true;
}

bool Xl95x5::Deinit(bool delete_bus) {
  bool result = true;

  if (!I2cChipBase::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Deinit failed\n");
    result = false;
  }

  if (rst_ != kPinNotConnected) {
    result &= PlatformHal::ResetGpio(rst_);
  }

  return result;
}

uint8_t Xl95x5::GetChipId() {
  uint8_t buffer = 0;

  if (!ReadRegister(static_cast<uint8_t>(Register::kRoChipId), &buffer)) {
    return kInvalidChipId;
  }

  return buffer;
}

bool Xl95x5::SetGpioMode(Pin pin, Mode mode) {
  uint8_t buffer = 0;

  if (pin == Pin::kIoPort0) {
    if (mode == Mode::kOutput) {
      buffer = 0B00000000;
    } else {
      buffer = 0B11111111;
    }
    if (!WriteRegister(
            static_cast<uint8_t>(Register::kRwConfigurationPort0), buffer)) {
      return false;
    }
  } else if (pin == Pin::kIoPort1) {
    if (mode == Mode::kOutput) {
      buffer = 0B00000000;
    } else {
      buffer = 0B11111111;
    }
    if (!WriteRegister(
            static_cast<uint8_t>(Register::kRwConfigurationPort1), buffer)) {
      return false;
    }
  } else if (static_cast<uint8_t>(pin) > 7) {
    if (!ReadRegister(
            static_cast<uint8_t>(Register::kRwConfigurationPort1), &buffer)) {
      return false;
    }
    if (mode == Mode::kOutput) {
      buffer = buffer & (~(1 << (static_cast<uint8_t>(pin) - 10)));
    } else {
      buffer = buffer | (1 << (static_cast<uint8_t>(pin) - 10));
    }
    if (!WriteRegister(
            static_cast<uint8_t>(Register::kRwConfigurationPort1), buffer)) {
      return false;
    }
  } else {
    if (!ReadRegister(
            static_cast<uint8_t>(Register::kRwConfigurationPort0), &buffer)) {
      return false;
    }
    if (mode == Mode::kOutput) {
      buffer = buffer & (~(1 << static_cast<uint8_t>(pin)));
    } else {
      buffer = buffer | (1 << static_cast<uint8_t>(pin));
    }
    if (!WriteRegister(
            static_cast<uint8_t>(Register::kRwConfigurationPort0), buffer)) {
      return false;
    }
  }

  return true;
}

bool Xl95x5::GpioWrite(Pin pin, uint8_t value) {
  uint8_t buffer = 0;

  if (pin == Pin::kIoPort0) {
    if (!WriteRegister(
            static_cast<uint8_t>(Register::kRwOutputPort0), value)) {
      return false;
    }
  } else if (pin == Pin::kIoPort1) {
    if (!WriteRegister(
            static_cast<uint8_t>(Register::kRwOutputPort1), value)) {
      return false;
    }
  } else if (static_cast<uint8_t>(pin) > 7) {
    if (!ReadRegister(
            static_cast<uint8_t>(Register::kRwOutputPort1), &buffer)) {
      return false;
    }
    if (value == 0) {
      buffer = buffer & (~(1 << (static_cast<uint8_t>(pin) - 10)));
    } else {
      buffer = buffer | (1 << (static_cast<uint8_t>(pin) - 10));
    }

    if (!WriteRegister(
            static_cast<uint8_t>(Register::kRwOutputPort1), buffer)) {
      return false;
    }
  } else {
    if (!ReadRegister(
            static_cast<uint8_t>(Register::kRwOutputPort0), &buffer)) {
      return false;
    }
    if (value == 0) {
      buffer = buffer & (~(1 << static_cast<uint8_t>(pin)));
    } else {
      buffer = buffer | (1 << static_cast<uint8_t>(pin));
    }
    if (!WriteRegister(
            static_cast<uint8_t>(Register::kRwOutputPort0), buffer)) {
      return false;
    }
  }

  return true;
}

uint8_t Xl95x5::GpioRead(Pin pin) {
  uint8_t buffer = 0;

  if (pin == Pin::kIoPort0) {
    if (!ReadRegister(
            static_cast<uint8_t>(Register::kRoInputPort0), &buffer)) {
      return -1;
    }
  } else if (pin == Pin::kIoPort1) {
    if (!ReadRegister(
            static_cast<uint8_t>(Register::kRoInputPort1), &buffer)) {
      return -1;
    }
  } else if (static_cast<uint8_t>(pin) > 7) {
    if (!ReadRegister(
            static_cast<uint8_t>(Register::kRoInputPort1), &buffer)) {
      return -1;
    }
    buffer = (buffer >> (static_cast<uint8_t>(pin) - 10)) & 0B00000001;
  } else {
    if (!ReadRegister(
            static_cast<uint8_t>(Register::kRoInputPort0), &buffer)) {
      return -1;
    }
    buffer = (buffer >> static_cast<uint8_t>(pin)) & 0B00000001;
  }

  return buffer;
}

bool Xl95x5::ClearIrqFlag() {
  uint8_t buffer = 0;

  for (uint8_t i = 0; i < 2; i++) {
    if (!ReadRegister(
            static_cast<uint8_t>(
                static_cast<uint8_t>(Register::kRoInputPort0) + i),
            &buffer)) {
      return false;
    }
  }

  return true;
}

bool Xl95x5::ReadRegister(uint8_t reg, uint8_t* data, size_t length) {
  const uint8_t register_packet[] = {reg};

  if (bus_ != nullptr &&
      bus_->WriteRead(register_packet, sizeof(register_packet), data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "XL95x5 register read failed (register: %#X)\n",
      static_cast<unsigned>(reg));
  return false;
}

bool Xl95x5::WriteRegister(uint8_t reg, uint8_t value) {
  const uint8_t register_packet[] = {reg, value};

  if (bus_ != nullptr &&
      bus_->Write(register_packet, sizeof(register_packet))) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "XL95x5 register write failed (register: %#X)\n",
      static_cast<unsigned>(reg));
  return false;
}
}  // namespace cpp_bus_driver
