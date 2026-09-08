/*
 * @Description: SGM41562S和SGM41562SA扩展寄存器布局实现
 * @Author: LILYGO_L
 * @License: GPL 3.0
 */
#include "chip/i2c/sgm41562xx/sgm41562xx.h"

namespace cpp_bus_driver {
namespace {
using Feature = Sgm41562xx::Feature;

// 当前型号组支持的可选配置功能。
constexpr std::initializer_list<Feature> kFeatures = {
    Feature::kPrechargeCurrent,
    Feature::kInputOvervoltageThreshold,
};

constexpr uint8_t kWatchdogMask = 0x60;
constexpr uint8_t kTerminationMultiplierMask = 0x04;
constexpr uint8_t kPrechargeMultiplierMask = 0x08;
constexpr uint8_t kPrechargeThresholdMask = 0x02;
constexpr uint8_t kPcbProtectionDisableMask = 0x40;
constexpr uint8_t kInputOvervoltageSelectMask = 0x20;
}  // namespace

Sgm41562xx::Sgm41562sDriver::Sgm41562sDriver() : ModelDriver(kFeatures) {}

bool Sgm41562xx::Sgm41562sDriver::Init(Sgm41562xx& chip) const {
  static constexpr uint8_t kInitSequence[] = {
      // 禁用看门狗。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kChargeTerminationTimerControl),
      0x1A,

      // 输入限流设为 980 mA。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kExtendedInputCurrentControl),
      0xFA,

      // 完成其他配置后开启充电。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kPowerOnConfiguration),
      0xA4,
  };
  return chip.InitSequence(kInitSequence, sizeof(kInitSequence));
}

bool Sgm41562xx::Sgm41562sDriver::SetChargeVoltageLimit(
    Sgm41562xx& chip, uint16_t voltage_mv) const {
  constexpr RegisterField kField = {
      Register::kChargeVoltageControl, 0xFE, 1, 3500, 4770, 10};
  return chip.SetRegisterField(kField, voltage_mv);
}

bool Sgm41562xx::Sgm41562sDriver::SetSystemRegulationVoltage(
    Sgm41562xx& chip, uint16_t voltage_mv) const {
  constexpr RegisterField kField = {
      Register::kSystemVoltageRegulation, 0x1F, 0, 3600, 5150, 50};
  return chip.SetRegisterField(kField, voltage_mv);
}

bool Sgm41562xx::Sgm41562sDriver::SetThermalRegulationThreshold(
    Sgm41562xx& chip, uint8_t temperature_c) const {
  constexpr RegisterField kField = {
      Register::kSystemVoltageRegulation, 0x60, 5, 60, 120, 20};
  return chip.SetRegisterField(kField, temperature_c);
}

bool Sgm41562xx::Sgm41562sDriver::SetFastChargeCurrentLimit(
    Sgm41562xx& chip, uint16_t current_ma) const {
  uint8_t miscellaneous_configuration = 0;
  if (!chip.ReadRegister(Register::kI2cAddressMiscellaneousConfiguration,
          miscellaneous_configuration)) {
    return false;
  }
  constexpr RegisterField kField = {
      Register::kChargeCurrentControl, 0x7F, 0, 8, 1024, 8};
  const uint16_t scale = (miscellaneous_configuration & 0x01) != 0 ? 4 : 1;
  if (current_ma > kField.maximum / scale) {
    return false;
  }
  return chip.SetRegisterField(kField, current_ma * scale);
}

bool Sgm41562xx::Sgm41562sDriver::SetWatchdogTimer(
    Sgm41562xx& chip, uint16_t timeout_s) const {
  uint8_t setting = 0;
  switch (timeout_s) {
    case 0:
      break;
    case 64:
      setting = 1;
      break;
    case 128:
      setting = 2;
      break;
    case 256:
      setting = 3;
      break;
    default:
      return false;
  }
  return chip.UpdateRegisterBits(Register::kChargeTerminationTimerControl,
      kWatchdogMask, static_cast<uint8_t>(setting << 5));
}

bool Sgm41562xx::Sgm41562sDriver::SetPrechargeToFastChargeThreshold(
    Sgm41562xx& chip, uint16_t voltage_mv) const {
  return chip.UpdateRegisterBits(Register::kExtendedInputCurrentControl,
      kPrechargeThresholdMask,
      voltage_mv == 3000 ? kPrechargeThresholdMask : 0x00);
}

bool Sgm41562xx::Sgm41562sDriver::ResetWatchdogTimer(Sgm41562xx& chip) const {
  return chip.UpdateRegisterBits(
      Register::kExtendedInputCurrentControl, 0x01, 0x01);
}

bool Sgm41562xx::Sgm41562sDriver::SetInputVoltageLoopEnable(
    Sgm41562xx& chip, bool enable) const {
  return chip.UpdateRegisterBits(
      Register::kSystemVoltageRegulation, 0x80, enable ? 0x00 : 0x80);
}

bool Sgm41562xx::Sgm41562sDriver::SetPcbOvertemperatureProtectionEnable(
    Sgm41562xx& chip, bool enable) const {
  return chip.UpdateRegisterBits(Register::kSystemStatus,
      kPcbProtectionDisableMask, enable ? 0x00 : kPcbProtectionDisableMask);
}

bool Sgm41562xx::Sgm41562sDriver::SetInputCurrentLimit(
    Sgm41562xx& chip, uint16_t current_ma) const {
  constexpr RegisterField kField = {
      Register::kExtendedInputCurrentControl, 0xF8, 3, 50, 980, 30};
  return chip.SetRegisterField(kField, current_ma);
}

bool Sgm41562xx::Sgm41562sDriver::FinishTerminationCurrentLimit(
    Sgm41562xx& chip) const {
  return chip.UpdateRegisterBits(
      Register::kExtendedCurrentControl, kTerminationMultiplierMask, 0x00);
}

bool Sgm41562xx::Sgm41562sDriver::SetPrechargeCurrentLimit(
    Sgm41562xx& chip, uint16_t current_ma) const {
  constexpr RegisterField kField = {
      Register::kExtendedCurrentControl, 0xF0, 4, 1, 31, 2};
  return chip.SetRegisterField(kField, current_ma) &&
         chip.UpdateRegisterBits(
             Register::kExtendedCurrentControl, kPrechargeMultiplierMask, 0x00);
}

bool Sgm41562xx::Sgm41562sDriver::SetInputOvervoltageThreshold(
    Sgm41562xx& chip, uint16_t voltage_mv) const {
  if (voltage_mv != 6000 && voltage_mv != 19000) {
    return false;
  }
  return chip.UpdateRegisterBits(Register::kSystemStatus,
      kInputOvervoltageSelectMask,
      voltage_mv == 19000 ? kInputOvervoltageSelectMask : 0x00);
}

bool Sgm41562xx::Sgm41562sDriver::ReadInputConfig(Sgm41562xx& chip,
    uint8_t input_source_control, uint8_t system_status,
    ChargerConfig& config) const {
  uint8_t extended_input_current_control = 0;
  if (!chip.ReadRegister(Register::kExtendedInputCurrentControl,
          extended_input_current_control)) {
    return false;
  }
  config.input_current_limit_ma =
      50 + 30 * ((extended_input_current_control >> 3) & 0x1F);
  config.exit_shipping_mode_interrupt_delay_ms =
      (input_source_control & 0x04) != 0 ? 100 : 2000;
  config.exit_shipping_mode_input_delay_ms =
      (input_source_control & 0x02) != 0 ? 100 : 2000;
  config.termination_deglitch_time_ms =
      (input_source_control & 0x01) != 0 ? 40 : 200;
  config.shipping_mode_interrupt_enabled =
      (extended_input_current_control & 0x04) == 0;
  config.precharge_to_fast_charge_threshold_mv =
      (extended_input_current_control & kPrechargeThresholdMask) != 0 ? 3000
                                                                      : 2800;
  config.input_overvoltage_threshold_mv =
      (system_status & kInputOvervoltageSelectMask) != 0 ? 19000 : 6000;
  config.pcb_overtemperature_protection_enabled =
      (system_status & kPcbProtectionDisableMask) == 0;
  return true;
}

bool Sgm41562xx::Sgm41562sDriver::ReadChargeConfig(Sgm41562xx& chip,
    uint8_t charge_current_control, uint8_t charge_voltage_control,
    ChargerConfig& config) const {
  constexpr RegisterField kChargeCurrent = {
      Register::kChargeCurrentControl, 0x7F, 0, 8, 1024, 8};
  constexpr RegisterField kChargeVoltage = {
      Register::kChargeVoltageControl, 0xFE, 1, 3500, 4770, 10};
  config.fast_charge_current_ma =
      DecodeRegisterField(kChargeCurrent, charge_current_control);
  if (config.quarter_charge_current_scale_enabled) {
    config.fast_charge_current_ma /= 4;
  }
  config.charge_voltage_limit_mv =
      DecodeRegisterField(kChargeVoltage, charge_voltage_control);

  uint8_t extended_current_control = 0;
  if (!chip.ReadRegister(
          Register::kExtendedCurrentControl, extended_current_control)) {
    return false;
  }
  config.termination_current_multiplier_six_enabled =
      (extended_current_control & kTerminationMultiplierMask) != 0;
  if (config.termination_current_multiplier_six_enabled) {
    config.termination_current_ma *= 6;
  }
  config.precharge_current_multiplier_six_enabled =
      (extended_current_control & kPrechargeMultiplierMask) != 0;
  config.precharge_current_ma =
      1 + 2 * ((extended_current_control >> 4) & 0x0F);
  if (config.precharge_current_multiplier_six_enabled) {
    config.precharge_current_ma *= 6;
  }
  return true;
}

void Sgm41562xx::Sgm41562sDriver::ParseProtectionConfig(
    uint8_t charge_timer_control, uint8_t system_voltage_regulation,
    ChargerConfig& config) const {
  constexpr RegisterField kSystemVoltage = {
      Register::kSystemVoltageRegulation, 0x1F, 0, 3600, 5150, 50};
  constexpr RegisterField kThermalRegulation = {
      Register::kSystemVoltageRegulation, 0x60, 5, 60, 120, 20};
  config.system_voltage_regulation_mv =
      DecodeRegisterField(kSystemVoltage, system_voltage_regulation);
  config.input_voltage_loop_enabled = (system_voltage_regulation & 0x80) == 0;
  config.thermal_regulation_threshold_c = static_cast<uint8_t>(
      DecodeRegisterField(kThermalRegulation, system_voltage_regulation));
  const uint8_t watchdog_setting = (charge_timer_control & kWatchdogMask) >> 5;
  config.watchdog_enabled = watchdog_setting != 0;
  if (config.watchdog_enabled) {
    config.watchdog_timeout_s = 64U << (watchdog_setting - 1);
  }
}

}  // namespace cpp_bus_driver
