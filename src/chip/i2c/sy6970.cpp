/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-05-16 10:19:45
 * @License: GPL 3.0
 */
#include "sy6970.h"

namespace cpp_bus_driver {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
constexpr const uint8_t Sy6970::kInitSequence[];
#endif

bool Sy6970::Init(int32_t freq_hz) {
  if (rst_ != kDefaultValue) {
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
        "Get sy6970 id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get sy6970 id success (id: %#X)\n", buffer);
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  return true;
}

bool Sy6970::Deinit(bool delete_bus) {
  if (!ChipI2cGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  if (rst_ != kDefaultValue) {
    ResetGpio(rst_);
  }

  return true;
}

uint8_t Sy6970::GetDeviceId() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDeviceControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return static_cast<uint8_t>(-1);
  }

  return (buffer & 0x38) >> 3;
}

bool Sy6970::SetHizModeEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwInputSourceControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x80;
  } else {
    buffer &= 0x7F;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwInputSourceControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetIlimPinEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwInputSourceControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x40;
  } else {
    buffer &= 0xBF;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwInputSourceControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetInputCurrentLimit(uint16_t current_ma) {
  if (current_ma < 100 || current_ma > 3250) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwInputSourceControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xC0) | static_cast<uint8_t>((current_ma - 100) / 50);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwInputSourceControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBoostHotThreshold(uint8_t threshold) {
  if (threshold > 3) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwTemperatureMonitorControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0x3F) | static_cast<uint8_t>(threshold << 6);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwTemperatureMonitorControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBoostColdThreshold(bool threshold) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwTemperatureMonitorControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (threshold) {
    buffer |= 0x20;
  } else {
    buffer &= 0xDF;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwTemperatureMonitorControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetVindpmOffset(uint16_t offset_mv) {
  if (offset_mv > 3100) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwTemperatureMonitorControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xE0) | static_cast<uint8_t>(offset_mv / 100);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwTemperatureMonitorControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetAdcConversionEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSystemControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x80;
  } else {
    buffer &= 0x7F;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSystemControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetAdcConversionRate(bool continuous) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSystemControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (continuous) {
    buffer |= 0x40;
  } else {
    buffer &= 0xBF;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSystemControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBoostFrequency(bool high_freq) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSystemControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (high_freq) {
    buffer &= 0xDF;
  } else {
    buffer |= 0x20;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSystemControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetAdaptiveCurrentLimitEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSystemControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x10;
  } else {
    buffer &= 0xEF;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSystemControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetHvdcpEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSystemControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x08;
  } else {
    buffer &= 0xF7;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSystemControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetHvdcpVoltageType(bool high_voltage) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSystemControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (high_voltage) {
    buffer |= 0x04;
  } else {
    buffer &= 0xFB;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSystemControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetForceDpdmDetection(bool force) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSystemControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (force) {
    buffer |= 0x02;
  } else {
    buffer &= 0xFD;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSystemControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetAutoDpdmDetectionEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSystemControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x01;
  } else {
    buffer &= 0xFE;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSystemControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBatteryLoadEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x80;
  } else {
    buffer &= 0x7F;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetWatchdogReset(bool reset) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (reset) {
    buffer |= 0x40;
  } else {
    buffer &= 0xBF;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetOtgEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x20;
  } else {
    buffer &= 0xDF;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetChargeEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x10;
  } else {
    buffer &= 0xEF;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetMinSystemVoltageLimit(uint16_t voltage_mv) {
  if (voltage_mv < 3000 || voltage_mv > 3700) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xF1) |
           static_cast<uint8_t>(((voltage_mv - 3000) / 100) << 1);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetPumpxEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeCurrentControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x80;
  } else {
    buffer &= 0x7F;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeCurrentControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetFastChargeCurrentLimit(uint16_t current_ma) {
  if (current_ma > 5056) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeCurrentControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0x80) | static_cast<uint8_t>(current_ma / 64);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeCurrentControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetPrechargeCurrentLimit(uint16_t current_ma) {
  if (current_ma < 64 || current_ma > 1024) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPrechrgTermCurrentControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0x0F) |
           static_cast<uint8_t>(((current_ma - 64) / 64) << 4);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPrechrgTermCurrentControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetTerminationCurrentLimit(uint16_t current_ma) {
  if (current_ma < 64 || current_ma > 1024) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPrechrgTermCurrentControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xF0) | static_cast<uint8_t>((current_ma - 64) / 64);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPrechrgTermCurrentControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetChargeVoltageLimit(uint16_t voltage_mv) {
  if (voltage_mv < 3840 || voltage_mv > 4608) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeVoltageControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0x03) |
           static_cast<uint8_t>(((voltage_mv - 3840) / 16) << 2);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeVoltageControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBatteryLowVoltageThreshold(bool high_threshold) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeVoltageControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (high_threshold) {
    buffer |= 0x02;
  } else {
    buffer &= 0xFD;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeVoltageControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBatteryRechargeThreshold(bool high_threshold) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeVoltageControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (high_threshold) {
    buffer |= 0x01;
  } else {
    buffer &= 0xFE;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeVoltageControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetChargeTerminationEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x80;
  } else {
    buffer &= 0x7F;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetStatPinDisable(bool disable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (disable) {
    buffer |= 0x40;
  } else {
    buffer &= 0xBF;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetWatchdogTimer(uint16_t timer_s) {
  uint8_t value = 0;
  switch (timer_s) {
    case 0:
      value = 0;
      break;
    case 40:
      value = 1;
      break;
    case 80:
      value = 2;
      break;
    case 160:
      value = 3;
      break;
    default:
      return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xCF) | static_cast<uint8_t>(value << 4);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetChargeSafetyTimerEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x08;
  } else {
    buffer &= 0xF7;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetFastChargeTimer(uint8_t timer_hr) {
  uint8_t value = 0;
  switch (timer_hr) {
    case 5:
      value = 0;
      break;
    case 8:
      value = 1;
      break;
    case 12:
      value = 2;
      break;
    case 20:
      value = 3;
      break;
    default:
      return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xF9) | static_cast<uint8_t>(value << 1);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetJeitaLowTempCurrent(bool low_current) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (low_current) {
    buffer |= 0x01;
  } else {
    buffer &= 0xFE;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBatteryCompensationResistance(uint8_t resistance_mohm) {
  if (resistance_mohm > 140) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwIrCompensationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0x1F) | static_cast<uint8_t>((resistance_mohm / 20) << 5);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwIrCompensationControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetIrCompensationVoltageClamp(uint8_t voltage_mv) {
  if (voltage_mv > 224) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwIrCompensationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xE3) | static_cast<uint8_t>((voltage_mv / 32) << 2);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwIrCompensationControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetThermalRegulationThreshold(uint8_t temperature) {
  uint8_t value = 0;
  switch (temperature) {
    case 60:
      value = 0;
      break;
    case 80:
      value = 1;
      break;
    case 100:
      value = 2;
      break;
    case 120:
      value = 3;
      break;
    default:
      return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwIrCompensationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xFC) | value;

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwIrCompensationControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetForceAdaptiveCurrentLimit(bool force) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (force) {
    buffer |= 0x80;
  } else {
    buffer &= 0x7F;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetSafetyTimerSlowdown(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x40;
  } else {
    buffer &= 0xBF;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetShippingModeEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x20;
  } else {
    buffer &= 0xDF;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetJeitaHighTempVoltage(bool normal_voltage) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (normal_voltage) {
    buffer |= 0x10;
  } else {
    buffer &= 0xEF;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBatfetTurnoffDelay(bool delay) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (delay) {
    buffer |= 0x08;
  } else {
    buffer &= 0xF7;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBatfetResetEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0x04;
  } else {
    buffer &= 0xFB;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetPumpControl(bool up, bool down) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (up) {
    buffer |= 0x02;
  } else {
    buffer &= 0xFD;
  }

  if (down) {
    buffer |= 0x01;
  } else {
    buffer &= 0xFE;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl),
          buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBoostVoltage(uint16_t voltage_mv) {
  if (voltage_mv < 4550 || voltage_mv > 5510) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwBoostModeControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0x0F) |
           static_cast<uint8_t>(((voltage_mv - 4550) / 64) << 4);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwBoostModeControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetBoostCurrentLimit(uint16_t current_ma) {
  uint8_t value = 0;
  switch (current_ma) {
    case 500:
      value = 0;
      break;
    case 750:
      value = 1;
      break;
    case 1200:
      value = 2;
      break;
    case 1400:
      value = 3;
      break;
    case 1650:
      value = 4;
      break;
    case 1875:
      value = 5;
      break;
    case 2150:
      value = 6;
      break;
    case 2450:
      value = 7;
      break;
    default:
      return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwBoostModeControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xF8) | value;

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwBoostModeControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetVindpmMode(bool absolute) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwVindpmControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (absolute) {
    buffer |= 0x80;
  } else {
    buffer &= 0x7F;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwVindpmControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::SetAbsoluteVindpmThreshold(uint16_t voltage_mv) {
  if (voltage_mv < 3900 || voltage_mv > 15300) {
    return false;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwVindpmControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0x80) | static_cast<uint8_t>((voltage_mv - 2600) / 100);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwVindpmControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint8_t Sy6970::GetChipStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdSystemStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return static_cast<uint8_t>(-1);
  }

  return buffer;
}

bool Sy6970::ParseChipStatus(uint8_t chip_flag, ChipStatus& status) {
  if (chip_flag == static_cast<uint8_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  status.bus_status = static_cast<BusStatus>((chip_flag & 0xE0) >> 5);
  status.charge_status = static_cast<ChargeStatus>((chip_flag & 0x18) >> 3);
  status.power_good_status = (chip_flag & 0x04) >> 2;
  status.usb_status = (chip_flag & 0x02) >> 1;
  status.system_voltage_regulation_status = chip_flag & 0x01;

  return true;
}

Sy6970::BusStatus Sy6970::ReadBusStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdSystemStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return BusStatus::kNoInput;
  }

  return static_cast<BusStatus>((buffer & 0xE0) >> 5);
}

Sy6970::ChargeStatus Sy6970::ReadChargeStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdSystemStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return ChargeStatus::kNotCharging;
  }

  return static_cast<ChargeStatus>((buffer & 0x18) >> 3);
}

bool Sy6970::ReadPowerGoodStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdSystemStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0x04) >> 2;
}

bool Sy6970::ReadUsbStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdSystemStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0x02) >> 1;
}

bool Sy6970::ReadSystemVoltageRegulationStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdSystemStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return buffer & 0x01;
}

uint8_t Sy6970::GetIrqFlag() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdFaultStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return static_cast<uint8_t>(-1);
  }

  return buffer;
}

bool Sy6970::ParseIrqStatus(uint8_t irq_flag, IrqStatus& status) {
  if (irq_flag == static_cast<uint8_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  status.watchdog_expiration_flag = (irq_flag & 0x80) >> 7;
  status.boost_fault_flag = (irq_flag & 0x40) >> 6;
  status.charge_fault_status =
      static_cast<ChargeFault>((irq_flag & 0x30) >> 4);
  status.battery_over_voltage_fault_flag = (irq_flag & 0x08) >> 3;
  status.ntc_fault_status = static_cast<NtcFault>(irq_flag & 0x07);

  return true;
}

bool Sy6970::ReadThermalRegulationStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdBatteryVoltage), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0x80) >> 7;
}

uint16_t Sy6970::GetBatteryVoltage() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdBatteryVoltage), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return 0;
  }

  return 2304 + ((buffer & 0x7F) * 20);
}

uint16_t Sy6970::GetSystemVoltage() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdSystemVoltage), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return 0;
  }

  return 2304 + ((buffer & 0x7F) * 20);
}

uint32_t Sy6970::ReadNtcVoltagePercentage() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdNtcVoltage), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return 0;
  }

  return 21000 + ((buffer & 0x7F) * 465);
}

bool Sy6970::ReadBusConnectionStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdBusVoltageStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0x80) >> 7;
}

uint16_t Sy6970::GetBusVoltage() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdBusVoltageStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return 0;
  }

  return 2600 + ((buffer & 0x7F) * 100);
}

uint16_t Sy6970::GetChargingCurrent() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRdChargeCurrent), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return 0;
  }

  return (buffer & 0x7F) * 50;
}

bool Sy6970::ReadVindpmStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRdInputCurrentLimitStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0x80) >> 7;
}

bool Sy6970::ReadIindpmStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRdInputCurrentLimitStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0x40) >> 6;
}

uint16_t Sy6970::ReadInputCurrentLimitSetting() {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRdInputCurrentLimitStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return 0;
  }

  return 100 + ((buffer & 0x3F) * 50);
}

bool Sy6970::SetRegisterReset(bool reset) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDeviceControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (reset) {
    buffer |= 0x80;
  } else {
    buffer &= 0x7F;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwDeviceControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sy6970::ReadAiclOptimizedStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDeviceControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0x40) >> 6;
}

uint8_t Sy6970::ReadDeviceConfiguration() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDeviceControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return 0;
  }

  return (buffer & 0x38) >> 3;
}

bool Sy6970::ReadTemperatureProfile() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDeviceControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0x04) >> 2;
}

uint8_t Sy6970::ReadDeviceRevision() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDeviceControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return 0;
  }

  return buffer & 0x03;
}
}  // namespace cpp_bus_driver
