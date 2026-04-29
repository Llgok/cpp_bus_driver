/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-04-20 17:06:28
 * @License: GPL 3.0
 */
#include "sgm41562xx.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
constexpr const uint8_t Sgm41562xx::kInitSequence[];
#endif

bool Sgm41562xx::Init(int32_t freq_hz) {
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
        "Get sgm41562xx id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get sgm41562xx id success (id: %#X)\n", buffer);
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  return true;
}

bool Sgm41562xx::Deinit() {
  if (!ChipI2cGuide::Deinit()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

uint8_t Sgm41562xx::GetDeviceId() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

uint8_t Sgm41562xx::GetIrqFlag() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdFault), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Sgm41562xx::ParseIrqStatus(uint8_t irq_flag, IrqStatus& status) {
  if (irq_flag == static_cast<uint8_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  status.InputPowerFaultFlag = (irq_flag & 0B00100000) >> 5;
  status.thermal_shutdown_flag = (irq_flag & 0B00010000) >> 4;
  status.battery_over_voltage_fault_flag = (irq_flag & 0B00001000) >> 3;
  status.safety_timer_expiration_fault_flag = (irq_flag & 0B00000100) >> 2;
  status.ntc_exceeding_hot_flag = (irq_flag & 0B00000010) >> 1;
  status.ntc_exceeding_cold_flag = irq_flag & 0B00000001;

  return true;
}

bool Sgm41562xx::SetChargeEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer &= 0B11110111;
  } else {
    buffer |= 0B00001000;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint8_t Sgm41562xx::GetChipStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdSystemStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Sgm41562xx::ParseChipStatus(uint8_t chip_flag, ChipStatus& status) {
  if (chip_flag == static_cast<uint8_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  status.watchdog_expiration_flag = (chip_flag & 0B00100000) >> 5;
  status.charge_status =
      static_cast<ChargeStatus>((chip_flag & 0B00011000) >> 3);
  status.device_in_power_path_management_mode_flag =
      (chip_flag & 0B00000100) >> 2;
  status.input_power_status_flag = (chip_flag & 0B00000010) >> 1;
  status.thermal_regulation_status_flag = chip_flag & 0B00000001;

  return true;
}

bool Sgm41562xx::SetShippingModeEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0B00100000;
  } else {
    buffer &= 0B11011111;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sgm41562xx::SetEnterShippingTime(EnterShippingTime time) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdFault), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B00111111) | (static_cast<uint8_t>(time) << 6);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRdFault), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
