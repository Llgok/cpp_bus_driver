/*
 * @Description: SGM38121 多通道 LDO 稳压器驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-09-02 16:15:39
 * @License: GPL 3.0
 */
#include "chip/i2c/sgm38121.h"

namespace cpp_bus_driver {
bool Sgm38121::Init(int32_t freq_hz) {
  if (!I2cChipBase::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Init failed\n");
    return false;
  }

  auto buffer = GetChipId();
  if (buffer != kChipId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get sgm38121 chip id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get sgm38121 chip id success (id: %#X)\n", buffer);
  }

  return true;
}

bool Sgm38121::Deinit(bool delete_bus) {
  bool result = true;

  if (!I2cChipBase::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Deinit failed\n");
    result = false;
  }

  return result;
}

uint8_t Sgm38121::GetChipId() {
  uint8_t buffer = 0;

  if (!ReadRegister(static_cast<uint8_t>(Register::kRoChipId), &buffer)) {
    return -1;
  }

  return buffer;
}

bool Sgm38121::SetOutputVoltage(Channel channel, uint16_t voltage) {
  uint8_t buffer = 0;

  switch (channel) {
    case Channel::kDvdd1:
      if (voltage < 528) {
        LogMessage(
            LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
        voltage = 528;
      } else if (voltage > 1504) {
        LogMessage(
            LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
        voltage = 1504;
      }
      buffer = (voltage - 504) / 8;
      if (!WriteRegister(
              static_cast<uint8_t>(Register::kRwDvdd1OutputVoltageLevel),
              buffer)) {
        return false;
      }
      break;
    case Channel::kDvdd2:
      if (voltage < 528) {
        LogMessage(
            LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
        voltage = 528;
      } else if (voltage > 1504) {
        LogMessage(
            LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
        voltage = 1504;
      }
      buffer = (voltage - 504) / 8;
      if (!WriteRegister(
              static_cast<uint8_t>(Register::kRwDvdd2OutputVoltageLevel),
              buffer)) {
        return false;
      }
      break;
    case Channel::kAvdd1:
      if (voltage < 1504) {
        LogMessage(
            LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
        voltage = 1504;
      } else if (voltage > 3424) {
        LogMessage(
            LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
        voltage = 3424;
      }
      buffer = (voltage - 1384) / 8;
      if (!WriteRegister(
              static_cast<uint8_t>(Register::kRwAvdd1OutputVoltageLevel),
              buffer)) {
        return false;
      }
      break;
    case Channel::kAvdd2:
      if (voltage < 1504) {
        LogMessage(
            LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
        voltage = 1504;
      } else if (voltage > 3424) {
        LogMessage(
            LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
        voltage = 3424;
      }
      buffer = (voltage - 1384) / 8;
      if (!WriteRegister(
              static_cast<uint8_t>(Register::kRwAvdd2OutputVoltageLevel),
              buffer)) {
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
  if (!ReadRegister(
          static_cast<uint8_t>(Register::kRwEnableControl), &buffer)) {
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
  if (!WriteRegister(
          static_cast<uint8_t>(Register::kRwEnableControl), buffer)) {
    return false;
  }

  return true;
}

bool Sgm38121::ReadRegister(uint8_t reg, uint8_t* data, size_t length) {
  const uint8_t register_packet[] = {reg};

  if (bus_ != nullptr &&
      bus_->WriteRead(register_packet, sizeof(register_packet), data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "SGM38121 register read failed (register: %#X)\n",
      static_cast<unsigned>(reg));
  return false;
}

bool Sgm38121::WriteRegister(uint8_t reg, uint8_t value) {
  const uint8_t register_packet[] = {reg, value};

  if (bus_ != nullptr &&
      bus_->Write(register_packet, sizeof(register_packet))) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "SGM38121 register write failed (register: %#X)\n",
      static_cast<unsigned>(reg));
  return false;
}
}  // namespace cpp_bus_driver
