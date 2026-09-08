/*
 * @Description: SGM41562、SGM41562A和SGM41562B基础寄存器布局实现
 * @Author: LILYGO_L
 * @License: GPL 3.0
 */
#include "chip/i2c/sgm41562xx/sgm41562xx.h"

namespace cpp_bus_driver {
namespace {
using Feature = Sgm41562xx::Feature;

// 当前型号组支持的可选配置功能。
constexpr std::initializer_list<Feature> kFeatures = {
    Feature::kInputCurrentLimitRelease,
    Feature::kInputCurrentLimitOffset,
};

constexpr uint8_t kWatchdogMask = 0x60;
constexpr uint8_t kInputCurrentLimitReleaseMask = 0x40;
constexpr uint8_t kInputCurrentLimitAdd200Mask = 0x20;
constexpr uint8_t kPrechargeThresholdMask = 0x02;
constexpr uint8_t kPcbProtectionDisableMask = 0x80;
}  // namespace

Sgm41562xx::Sgm41562Driver::Sgm41562Driver() : ModelDriver(kFeatures) {}

bool Sgm41562xx::Sgm41562Driver::Init(Sgm41562xx& chip) const {
  static constexpr uint8_t kInitSequence[] = {
      // 禁用看门狗。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kChargeTerminationTimerControl),
      0x1A,

      // 输入限流设为 500 mA，输入电压限制保持默认 4.6 V。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kInputSourceControl),
      0x9F,

      // 限流解除和额外 200 mA 偏移均保持上电默认关闭。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kSystemStatus),
      0x00,

      // 完成其他配置后开启充电。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kPowerOnConfiguration),
      0xA4,
  };
  return chip.InitSequence(kInitSequence, sizeof(kInitSequence));
}

bool Sgm41562xx::Sgm41562Driver::SetChargeVoltageLimit(
    Sgm41562xx& chip, uint16_t voltage_mv) const {
  constexpr RegisterField kField = {
      Register::kChargeVoltageControl, 0xFC, 2, 3600, 4545, 15};
  return chip.SetRegisterField(kField, voltage_mv);
}

bool Sgm41562xx::Sgm41562Driver::SetSystemRegulationVoltage(
    Sgm41562xx& chip, uint16_t voltage_mv) const {
  constexpr RegisterField kField = {
      Register::kSystemVoltageRegulation, 0x0F, 0, 4200, 4950, 50};
  return chip.SetRegisterField(kField, voltage_mv);
}

bool Sgm41562xx::Sgm41562Driver::SetThermalRegulationThreshold(
    Sgm41562xx& chip, uint8_t temperature_c) const {
  constexpr RegisterField kField = {
      Register::kSystemVoltageRegulation, 0x30, 4, 60, 120, 20};
  return chip.SetRegisterField(kField, temperature_c);
}

bool Sgm41562xx::Sgm41562Driver::SetFastChargeCurrentLimit(
    Sgm41562xx& chip, uint16_t current_ma) const {
  uint8_t miscellaneous_configuration = 0;
  if (!chip.ReadRegister(Register::kI2cAddressMiscellaneousConfiguration,
          miscellaneous_configuration)) {
    return false;
  }
  constexpr RegisterField kField = {
      Register::kChargeCurrentControl, 0x3F, 0, 8, 456, 8};
  const uint16_t scale = (miscellaneous_configuration & 0x01) != 0 ? 4 : 1;
  if (current_ma > kField.maximum / scale) {
    return false;
  }
  return chip.SetRegisterField(kField, current_ma * scale);
}

bool Sgm41562xx::Sgm41562Driver::SetWatchdogTimer(
    Sgm41562xx& chip, uint16_t timeout_s) const {
  uint8_t setting = 0;
  switch (timeout_s) {
    case 0:
      break;
    case 40:
      setting = 1;
      break;
    case 80:
      setting = 2;
      break;
    case 160:
      setting = 3;
      break;
    default:
      return false;
  }
  return chip.UpdateRegisterBits(Register::kChargeTerminationTimerControl,
      kWatchdogMask, static_cast<uint8_t>(setting << 5));
}

bool Sgm41562xx::Sgm41562Driver::SetPrechargeToFastChargeThreshold(
    Sgm41562xx& chip, uint16_t voltage_mv) const {
  return chip.UpdateRegisterBits(Register::kChargeVoltageControl,
      kPrechargeThresholdMask,
      voltage_mv == 3000 ? kPrechargeThresholdMask : 0x00);
}

bool Sgm41562xx::Sgm41562Driver::ResetWatchdogTimer(Sgm41562xx& chip) const {
  return chip.UpdateRegisterBits(Register::kChargeCurrentControl, 0x40, 0x40);
}

bool Sgm41562xx::Sgm41562Driver::SetInputVoltageLoopEnable(
    Sgm41562xx& chip, bool enable) const {
  return chip.UpdateRegisterBits(
      Register::kSystemVoltageRegulation, 0x40, enable ? 0x00 : 0x40);
}

bool Sgm41562xx::Sgm41562Driver::SetPcbOvertemperatureProtectionEnable(
    Sgm41562xx& chip, bool enable) const {
  return chip.UpdateRegisterBits(Register::kSystemVoltageRegulation,
      kPcbProtectionDisableMask, enable ? 0x00 : kPcbProtectionDisableMask);
}

bool Sgm41562xx::Sgm41562Driver::SetInputCurrentLimit(
    Sgm41562xx& chip, uint16_t current_ma) const {
  constexpr RegisterField kField = {
      Register::kInputSourceControl, 0x0F, 0, 50, 500, 30};
  return chip.SetRegisterField(kField, current_ma) &&
         chip.UpdateRegisterBits(Register::kSystemStatus,
             kInputCurrentLimitReleaseMask | kInputCurrentLimitAdd200Mask,
             0x00);
}

bool Sgm41562xx::Sgm41562Driver::SetInputCurrentLimitReleaseEnable(
    Sgm41562xx& chip, bool enable) const {
  return chip.UpdateRegisterBits(Register::kSystemStatus,
      kInputCurrentLimitReleaseMask,
      enable ? kInputCurrentLimitReleaseMask : 0x00);
}

bool Sgm41562xx::Sgm41562Driver::SetInputCurrentLimitOffsetEnable(
    Sgm41562xx& chip, bool enable) const {
  return chip.UpdateRegisterBits(Register::kSystemStatus,
      kInputCurrentLimitAdd200Mask,
      enable ? kInputCurrentLimitAdd200Mask : 0x00);
}

bool Sgm41562xx::Sgm41562Driver::ReadInputConfig(Sgm41562xx& chip,
    uint8_t input_source_control, uint8_t system_status,
    ChargerConfig& config) const {
  config.input_current_limit_enabled =
      (system_status & kInputCurrentLimitReleaseMask) == 0;
  config.input_current_limit_200_ma_offset_enabled =
      (system_status & kInputCurrentLimitAdd200Mask) != 0;
  config.input_current_limit_ma = 50 + 30 * (input_source_control & 0x0F);
  if ((system_status & kInputCurrentLimitAdd200Mask) != 0) {
    config.input_current_limit_ma += 200;
  }
  config.input_overvoltage_threshold_mv =
      chip.chip_model_ == ChipModel::kSgm41562A ? 19000 : 6000;
  return true;
}

bool Sgm41562xx::Sgm41562Driver::ReadChargeConfig(Sgm41562xx&,
    uint8_t charge_current_control, uint8_t charge_voltage_control,
    ChargerConfig& config) const {
  constexpr RegisterField kChargeCurrent = {
      Register::kChargeCurrentControl, 0x3F, 0, 8, 456, 8};
  constexpr RegisterField kChargeVoltage = {
      Register::kChargeVoltageControl, 0xFC, 2, 3600, 4545, 15};
  config.fast_charge_current_ma =
      DecodeRegisterField(kChargeCurrent, charge_current_control);
  if (config.quarter_charge_current_scale_enabled) {
    config.fast_charge_current_ma /= 4;
  }
  config.charge_voltage_limit_mv =
      DecodeRegisterField(kChargeVoltage, charge_voltage_control);

  config.precharge_to_fast_charge_threshold_mv =
      (charge_voltage_control & kPrechargeThresholdMask) != 0 ? 3000 : 2800;
  return true;
}

void Sgm41562xx::Sgm41562Driver::ParseProtectionConfig(
    uint8_t charge_timer_control, uint8_t system_voltage_regulation,
    ChargerConfig& config) const {
  constexpr RegisterField kSystemVoltage = {
      Register::kSystemVoltageRegulation, 0x0F, 0, 4200, 4950, 50};
  constexpr RegisterField kThermalRegulation = {
      Register::kSystemVoltageRegulation, 0x30, 4, 60, 120, 20};
  config.system_voltage_regulation_mv =
      DecodeRegisterField(kSystemVoltage, system_voltage_regulation);
  config.input_voltage_loop_enabled = (system_voltage_regulation & 0x40) == 0;
  config.thermal_regulation_threshold_c = static_cast<uint8_t>(
      DecodeRegisterField(kThermalRegulation, system_voltage_regulation));
  const uint8_t watchdog_setting = (charge_timer_control & kWatchdogMask) >> 5;
  config.watchdog_enabled = watchdog_setting != 0;
  if (config.watchdog_enabled) {
    config.watchdog_timeout_s = 40U << (watchdog_setting - 1);
  }

  config.pcb_overtemperature_protection_enabled =
      (system_voltage_regulation & kPcbProtectionDisableMask) == 0;
}

}  // namespace cpp_bus_driver
