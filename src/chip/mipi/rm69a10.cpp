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
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    GpioWrite(rst_, 1);
    DelayMs(5);
    GpioWrite(rst_, 0);
    DelayMs(10);
    GpioWrite(rst_, 1);
    DelayMs(120);
  }

  if (!ChipMipiGuide::Init(freq_mhz, lane_bit_rate_mbps)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
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
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  if (!bus_->StartTransmit()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "StartTransmit failed\n");
    return false;
  }

  return true;
}

bool Rm69a10::Deinit() {
  if (!ChipMipiGuide::Deinit()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kDisable, GpioStatus::kDisable);
  }

  return true;
}

uint8_t Rm69a10::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer, 1)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Rm69a10::SetSleep(bool enable) {
  if (!bus_->Write(enable ? static_cast<uint8_t>(Cmd::kWoSlpin)
                          : static_cast<uint8_t>(Cmd::kWoSlpout))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  DelayMs(120);

  return true;
}

bool Rm69a10::SetScreenOff(bool enable) {
  if (!bus_->Write(enable ? static_cast<uint8_t>(Cmd::kWoDispoff)
                          : static_cast<uint8_t>(Cmd::kWoDispon))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Rm69a10::SetInversion(bool enable) {
  if (!bus_->Write(enable ? static_cast<uint8_t>(Cmd::kWoInvon)
                          : static_cast<uint8_t>(Cmd::kWoInvoff))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Rm69a10::SetBrightness(uint8_t brightness) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWrdisbv), brightness)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Rm69a10::SendColorStreamCoordinate(uint16_t x_start, uint16_t x_end,
    uint16_t y_start, uint16_t y_end, const void* data) {
  if (!bus_->Write(x_start, x_end, y_start, y_end, data)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}
}  // namespace cpp_bus_driver
