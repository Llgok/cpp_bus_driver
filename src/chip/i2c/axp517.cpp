/*
 * @Description: AXP517 电源管理、Fuel Gauge 与 Type-C/PD 控制器驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-09-18 16:30:00
 * @LastEditTime: 2026-09-19 09:58:00
 * @License: GPL 3.0
 */
#include "chip/i2c/axp517.h"

#include <algorithm>
#include <cmath>

namespace cpp_bus_driver {
namespace {
constexpr uint16_t kChargeVoltages[] = {4000, 4100, 4200, 4350,
                                      4400, 3600, 3800, 5000};
constexpr float kNtcTemperatures[] = {-25, -15, -10, -5, 0, 5, 10, 20,
                                     30, 40, 45, 50, 55, 60, 70, 80};
constexpr uint16_t kRxMessageAlert = 0x0004;
constexpr uint16_t kRxOverflowAlert = 0x0400;
// 普通中断有效位掩码。
constexpr uint64_t kValidIrqs = 0x03FFFFFFF7ULL;
// PD Alert 有效位掩码。
constexpr uint16_t kValidPdAlerts = 0xEFFF;
// Type-C 控制器厂商 ID。
constexpr uint16_t kVendorId = 0x1F3A;
// BUCK 功能使能位。
constexpr uint8_t kBuckEnable = 0x08;
// BOOST 功能使能位。
constexpr uint8_t kBoostEnable = 0x10;
// 电量计模型存储区访问使能位。
constexpr uint8_t kBromEnable = 0x01;
// 电量计模型更新标志位。
constexpr uint8_t kModelUpdated = 0x10;
// MCU 复位控制位。
constexpr uint8_t kMcuReset = 0x04;
// 电量计模型的最大长度，单位字节。
constexpr size_t kMaxBatteryModelSize = 128;

}  // namespace

bool Axp517::Init(int32_t freq_hz) {
  if (!I2cChipBase::Init(freq_hz)) return false;
  ChipId chip_id;
  if (!GetChipId(chip_id)) {
    I2cChipBase::Deinit(false);
    return false;
  }
  ntc_configured_ = false;
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
             "AXP517 chip ID: 0x%02X, extended ID: 0x%02X\n",
             chip_id.chip_id, chip_id.extended_id);
  return true;
}

bool Axp517::Deinit(bool delete_bus) {
  ntc_configured_ = false;
  return I2cChipBase::Deinit(delete_bus);
}

bool Axp517::GetChipId(ChipId& chip_id) {
  ChipId result;
  if (!ReadRegister(static_cast<uint8_t>(Register::kChipId), &result.chip_id) ||
      !ReadRegister(static_cast<uint8_t>(Register::kChipIdExt), &result.extended_id)) return false;
  chip_id = result;
  return true;
}

bool Axp517::GetStatus(Status& status) {
  uint8_t data[2];
  if (!ReadRegister(static_cast<uint8_t>(Register::kStatus0), data, sizeof(data))) return false;
  Status result;
  result.current_limited = (data[0] & 0x01) != 0;
  result.thermal_regulation = (data[0] & 0x02) != 0;
  result.battery_active = (data[0] & 0x04) != 0;
  result.battery_present = (data[0] & 0x08) != 0;
  result.batfet_on = (data[0] & 0x10) != 0;
  result.vbus_good = (data[0] & 0x20) != 0;
  result.vindpm_active = (data[1] & 0x08) != 0;
  result.system_on = (data[1] & 0x10) != 0;
  const uint8_t charge = data[1] & 0x07;
  result.charge = charge <= 5 ? static_cast<ChargeStatus>(charge)
                              : ChargeStatus::kInvalid;
  result.current_direction =
      static_cast<BatteryCurrentDirection>((data[1] >> 5) & 0x03);
  status = result;
  return true;
}

bool Axp517::GetFaultStatus(FaultStatus& status) {
  uint8_t ntc, fault;
  if (!ReadRegister(static_cast<uint8_t>(Register::kIlimType), &ntc) ||
      !ReadRegister(static_cast<uint8_t>(Register::kBmuFault1), &fault)) return false;
  FaultStatus result;
  const uint8_t code = (ntc >> 4) & 0x07;
  result.ntc = code == 0 || code == 1 || code == 2 || code == 5 || code == 6
                   ? static_cast<NtcFault>(code) : NtcFault::kUnknown;
  result.system_overvoltage = (fault & 0x08) != 0;
  result.battery_undervoltage = (fault & 0x04) != 0;
  status = result;
  return true;
}

bool Axp517::ClearFaults(uint8_t mask) {
  return (mask & ~0x0C) == 0 && WriteRegister(static_cast<uint8_t>(Register::kBmuFault1), mask);
}

bool Axp517::GetBc12Result(Bc12Result& result) {
  uint8_t value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kBcDetect), &value)) return false;
  value >>= 5;
  result = value >= 1 && value <= 3 ? static_cast<Bc12Result>(value)
                                   : Bc12Result::kUnknown;
  return true;
}

bool Axp517::SetBc12DetectEnable(bool enable) {
  return UpdateRegisterBits(Register::kClkEn, 0x10, enable ? 0x10 : 0);
}

bool Axp517::SetChargeEnable(bool enable) {
  if (enable) {
    return UpdateRegisterBits(Register::kModuleEn, 0x0A, 0x0A);
  }
  if (!UpdateRegisterBits(Register::kModuleEn, 0x0A, 0)) return false;
  DelayMs(1000);
  return SetBuckEnable(true);
}

bool Axp517::SetChargeCurrent(uint16_t current_ma) {
  const uint8_t code = std::min<uint16_t>(current_ma / 64, 80);
  return UpdateRegisterBits(Register::kIccCfg, 0x7F, code);
}

bool Axp517::GetChargeCurrent(uint16_t& current_ma) {
  uint8_t value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kIccCfg), &value)) return false;
  current_ma = std::min<unsigned>(value & 0x7F, 80) * 64;
  return true;
}

bool Axp517::SetChargeVoltage(uint16_t voltage_mv) {
  uint8_t code;
  if (voltage_mv < 4100) code = 0;
  else if (voltage_mv < 4200) code = 1;
  else if (voltage_mv < 4350) code = 2;
  else if (voltage_mv < 4400) code = 3;
  else if (voltage_mv < 5000) code = 4;
  else code = 7;
  return UpdateRegisterBits(Register::kVtermCfg, 0x07, code);
}

bool Axp517::GetChargeVoltage(uint16_t& voltage_mv) {
  uint8_t value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kVtermCfg), &value)) return false;
  voltage_mv = kChargeVoltages[value & 0x07];
  return true;
}

bool Axp517::SetPrechargeCurrent(uint16_t current_ma) {
  return UpdateRegisterBits(Register::kIprechgCfg, 0x0F,
                            std::min<uint16_t>(current_ma / 64, 15));
}

bool Axp517::SetTrickleCurrent(uint16_t current_ma) {
  if (current_ma < 32 || current_ma > 224) return false;
  return UpdateRegisterBits(Register::kIprechgCfg, 0x70,
                            (current_ma / 32) << 4);
}

bool Axp517::SetTerminationCurrent(uint16_t current_ma, bool enable) {
  const uint8_t code =
      std::max<uint16_t>(1, std::min<uint16_t>(15, current_ma / 64));
  return UpdateRegisterBits(Register::kItermCfg, 0x1F,
                            code | (enable ? 0x10 : 0));
}

bool Axp517::SetTerminationDisabledInDpm(bool disable) {
  return UpdateRegisterBits(Register::kItermCfg, 0x20, disable ? 0x20 : 0);
}

bool Axp517::SetInputCurrentLimit(uint16_t current_ma) {
  if (current_ma < 100) return SetBuckEnable(false);
  const uint8_t code = ((std::min<uint16_t>(current_ma, 3250) - 100) / 50) << 2;
  return UpdateRegisterBits(Register::kIinLim, 0xFC, code) &&
         SetBuckEnable(true);
}

bool Axp517::GetInputCurrentLimit(uint16_t& current_ma) {
  uint8_t module, value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kModuleEn), &module)) return false;
  if ((module & kBuckEnable) == 0) {
    current_ma = 0;
    return true;
  }
  if (!ReadRegister(static_cast<uint8_t>(Register::kIinLim), &value)) return false;
  current_ma = (value >> 2) * 50 + 100;
  return true;
}

bool Axp517::SetInputVoltageLimit(uint16_t voltage_mv) {
  voltage_mv = std::max<uint16_t>(3600, std::min<uint16_t>(16200, voltage_mv));
  return UpdateRegisterBits(Register::kVindpmCfg, 0x7F,
                            (voltage_mv - 3600) / 100 + 1);
}

bool Axp517::GetInputVoltageLimit(uint16_t& voltage_mv) {
  uint8_t value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kVindpmCfg), &value)) return false;
  voltage_mv = (std::max<unsigned>(1, value & 0x7F) - 1) * 100 + 3600;
  return true;
}

bool Axp517::ApplyNegotiatedInputVoltage(uint16_t voltage_mv,
                                        uint16_t default_vindpm_mv) {
  if (voltage_mv == 0) return false;
  return WriteRegister(static_cast<uint8_t>(Register::kSwChgCfg1),
                       voltage_mv >= 9000 ? 0x00 : 0x04) &&
         SetInputVoltageLimit(voltage_mv >= 9000 ? 5500 : default_vindpm_mv);
}

bool Axp517::ConfigureCharging(const ChargeProfile& profile) {
  if (profile.voltage_mv < 4000 || profile.voltage_mv > 5000 ||
      profile.runtime_current_ma > 5120 || profile.suspend_current_ma > 5120 ||
      profile.shutdown_current_ma > 5120 || profile.limited_current_ma > 5120 ||
      profile.precharge_current_ma > 960 ||
      profile.termination_current_ma < 64 ||
      profile.termination_current_ma > 960 || profile.warning_percent < 5 ||
      profile.warning_percent > 20 || profile.shutdown_percent > 15 ||
      profile.shutdown_percent > profile.warning_percent) return false;
  if (!SetChargeVoltage(profile.voltage_mv) ||
      !SetPrechargeCurrent(profile.precharge_current_ma) ||
      !SetTerminationCurrent(profile.termination_current_ma) ||
      !SetGaugeThresholds(profile.warning_percent, profile.shutdown_percent) ||
      !SetChargeCurrent(profile.runtime_current_ma)) return false;
  charge_profile_ = profile;
  return true;
}

bool Axp517::SetChargeMode(ChargeMode mode) {
  switch (mode) {
    case ChargeMode::kRuntime:
      return SetChargeCurrent(charge_profile_.runtime_current_ma);
    case ChargeMode::kSuspend:
      return SetChargeCurrent(charge_profile_.suspend_current_ma);
    case ChargeMode::kShutdown: {
      Status status;
      if (!GetStatus(status)) return false;
      if (status.battery_present) {
        uint8_t module;
        if (!ReadRegister(static_cast<uint8_t>(Register::kModuleEn), &module) ||
            !SetBuckEnable(false)) {
          return false;
        }
        DelayMs(100);
        if (!SetBuckEnable((module & kBuckEnable) != 0)) return false;
      }
      return SetChargeCurrent(charge_profile_.shutdown_current_ma);
    }
    case ChargeMode::kLimited:
      return SetChargeCurrent(charge_profile_.limited_current_ma);
    case ChargeMode::kPaused:
      return SetChargeCurrent(0);
  }
  return false;
}

bool Axp517::SetBuckEnable(bool enable) {
  return UpdateRegisterBits(Register::kModuleEn, kBuckEnable,
                            enable ? kBuckEnable : 0);
}

bool Axp517::SetBoostEnable(bool enable) {
  return UpdateRegisterBits(Register::kModuleEn, kBoostEnable,
                            enable ? kBoostEnable : 0);
}

bool Axp517::SetBoostVoltage(uint16_t voltage_mv) {
  voltage_mv = std::max<uint16_t>(4550, std::min<uint16_t>(5510, voltage_mv));
  return UpdateRegisterBits(Register::kBstCfg0, 0xF0,
                            ((voltage_mv - 4550) / 64) << 4);
}

bool Axp517::SetMinimumSystemVoltage(uint16_t voltage_mv) {
  if (voltage_mv < 1000 || voltage_mv > 3700) return false;
  return UpdateRegisterBits(Register::kVsysMin, 0x1F,
                            (voltage_mv - 1000) / 100);
}

bool Axp517::SetBoostDisableThreshold(uint16_t voltage_mv) {
  const uint8_t code = voltage_mv <= 2600
                           ? 3
                           : voltage_mv <= 2800
                                 ? 2
                                 : voltage_mv <= 3000
                                       ? 1
                                       : voltage_mv <= 3200 ? 0 : 0xFF;
  return code != 0xFF &&
         UpdateRegisterBits(Register::kBstCfg0, 0x0C, code << 2);
}

bool Axp517::SetBoostRbfetCurrentLimit(uint16_t current_ma) {
  const uint8_t code = current_ma <= 500
                           ? 0
                           : current_ma <= 900
                                 ? 1
                                 : current_ma <= 1500
                                       ? 2
                                       : current_ma <= 2000 ? 3 : 0xFF;
  return code != 0xFF && UpdateRegisterBits(Register::kBstCfg0, 0x03, code);
}

bool Axp517::SetRechargeThreshold(uint8_t code) {
  return code < 8 && UpdateRegisterBits(Register::kRechgCfg, 0x07, code);
}

bool Axp517::SetChargeFrequency(uint8_t code) {
  return code <= 0x0F && WriteRegister(static_cast<uint8_t>(Register::kChgFreq), code);
}

bool Axp517::SetChargeTimer(bool enable, uint8_t code) {
  return code <= 3 &&
         UpdateRegisterBits(Register::kChgTmrCfg, 0x83,
                            (enable ? 0x80 : 0) | code);
}

bool Axp517::SetBatteryIrCompensation(uint8_t resistance_code,
                                       uint8_t voltage_code) {
  return resistance_code <= 3 && voltage_code <= 7 &&
      WriteRegister(static_cast<uint8_t>(Register::kIrComp),
                    (resistance_code << 6) | (voltage_code & 0x07));
}

bool Axp517::SetSwitchChargeConfiguration(uint8_t value) {
  return WriteRegister(static_cast<uint8_t>(Register::kSwChgCfg1), value);
}

bool Axp517::GetBoostVoltage(uint16_t& voltage_mv) {
  uint8_t value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kBstCfg0), &value)) return false;
  voltage_mv = 4550 + (value >> 4) * 64;
  return true;
}

bool Axp517::SetRbfetForceEnable(bool enable) {
  if (enable) return UpdateRegisterBits(Register::kRbfetCtrl, 0x01, 0x01);
  uint8_t irq_enable;
  if (!ReadRegister(static_cast<uint8_t>(Register::kIrqEn1), &irq_enable) ||
      !UpdateRegisterBits(Register::kIrqEn1, 0xC0, 0)) return false;
  bool success = UpdateRegisterBits(Register::kRbfetCtrl, 0x01, 0);
  // W1C 必须直接写入，避免官方读改写误清除其他电池/按键事件。
  if (success) success = WriteRegister(static_cast<uint8_t>(Register::kIrq1), 0xC0);
  DelayMs(10);
  if (success) success = WriteRegister(static_cast<uint8_t>(Register::kIrq1), 0xC0);
  const bool restored = UpdateRegisterBits(Register::kIrqEn1, 0xC0, irq_enable);
  return success && restored;
}

bool Axp517::SetBatfetMode(BatfetMode mode) {
  if (mode != BatfetMode::kAuto && mode != BatfetMode::kOn &&
      mode != BatfetMode::kOff) {
    return false;
  }
  return UpdateRegisterBits(Register::kBatfetCtrl, 0x05,
                            static_cast<uint8_t>(mode));
}

bool Axp517::SetShippingModeEnable(bool enable) {
  return UpdateRegisterBits(Register::kBatfetCtrl, 0x08, enable ? 0x08 : 0);
}

bool Axp517::SetWatchdog(bool enable, uint8_t timeout_s) {
  uint8_t code = 0;
  while (code < 7 && (1U << code) < timeout_s) ++code;
  // 禁用时先停模块，启用时先开时钟；避免在无时钟情况下启动看门狗。
  if (!enable && !UpdateRegisterBits(Register::kModuleEn, 0x01, 0)) {
    return false;
  }
  return UpdateRegisterBits(Register::kWatchdogCfg, 0x07, code) &&
         UpdateRegisterBits(Register::kClkEn, 0x01, enable ? 0x01 : 0) &&
         (!enable || UpdateRegisterBits(Register::kModuleEn, 0x01, 0x01));
}

bool Axp517::FeedWatchdog() {
  return UpdateRegisterBits(Register::kWatchdogCfg, 0x08, 0x08);
}

bool Axp517::SetLongPressResetEnable(bool enable) {
  return UpdateRegisterBits(Register::kResetCfg, 0x02, enable ? 0x02 : 0);
}

bool Axp517::SetThermalRegulationThreshold(uint8_t celsius) {
  const uint8_t code =
      celsius > 100 ? 3 : celsius > 80 ? 2 : celsius > 60 ? 1 : 0;
  return UpdateRegisterBits(Register::kTreguThld, 0xC0, code << 6);
}

bool Axp517::SetThermalShutdown(bool enable, uint8_t celsius) {
  const uint8_t code = celsius > 125 ? 2 : celsius > 115 ? 1 : 0;
  return UpdateRegisterBits(Register::kDieTempCfg, 0x0E,
                            (enable ? 0x08 : 0) | (code << 1));
}

bool Axp517::ConfigureChargeLed(bool enable, uint8_t function) {
  if (function > 7) return false;
  return UpdateRegisterBits(Register::kChgledCfg, 0x07, function) &&
         UpdateRegisterBits(Register::kModuleEn, 0x04, enable ? 0x04 : 0);
}

bool Axp517::SetBatteryDetectionEnable(bool enable) {
  return UpdateRegisterBits(Register::kBatDet, 0x01, enable ? 0x01 : 0);
}

bool Axp517::ConfigureGpio(GpioSource source, GpioMode mode,
                            GpioOutput output) {
  if (static_cast<uint8_t>(source) > 1 || static_cast<uint8_t>(mode) > 1 ||
      static_cast<uint8_t>(output) > 2) return false;
  return UpdateRegisterBits(
      Register::kGpioCfg, 0x1F,
      (static_cast<uint8_t>(source) << 2) |
          (mode == GpioMode::kOutput ? 0x10 : 0) |
          static_cast<uint8_t>(output));
}

bool Axp517::ReadGpio(bool& high) {
  uint8_t value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kGpioCfg), &value)) return false;
  high = (value & 0x02) != 0;
  return true;
}

bool Axp517::SetAdcChannels(uint8_t mask) {
  return WriteRegister(static_cast<uint8_t>(Register::kAdcChEn0), mask);
}

bool Axp517::SetAdcChannelEnable(AdcChannel channel, bool enable) {
  const uint8_t mask = static_cast<uint8_t>(channel);
  if (mask == 0 || (mask & (mask - 1)) != 0) return false;
  return UpdateRegisterBits(Register::kAdcChEn0, mask, enable ? mask : 0);
}

bool Axp517::ReadAdc(AdcInput input, uint16_t& raw) {
  const uint8_t channel = static_cast<uint8_t>(input);
  if (channel > 7) return false;
  uint8_t selection;
  if (!ReadRegister(static_cast<uint8_t>(Register::kAdcControl), &selection)) return false;
  if ((selection & 0x0F) != channel) {
    if (!WriteRegister(static_cast<uint8_t>(Register::kAdcControl), (selection & 0xF0) | channel)) {
      return false;
    }
    DelayMs(1);
  }
  return ReadBigEndian16(Register::kAdcRes, raw);
}

float Axp517::DecodeCurrent(uint16_t raw) {
  const int magnitude =
      (raw & 0x8000) ? ((~raw + 1) & 0x3FFF) : (raw & 0x3FFF);
  return (raw & 0x8000) ? -magnitude / 4.0f : magnitude / 4.0f;
}

bool Axp517::GetBatteryVoltage(uint16_t& voltage_mv) {
  uint16_t raw;
  if (!ReadBigEndian16(Register::kVbatH, raw)) return false;
  voltage_mv = raw & 0x3FFF;
  return true;
}

bool Axp517::GetBatteryCurrent(float& current_ma) {
  uint16_t raw;
  if (!ReadBigEndian16(Register::kIbatH, raw)) return false;
  current_ma = DecodeCurrent(raw);
  return true;
}

bool Axp517::ReadSelectedCurrent(AdcInput input, float& current_ma) {
  uint16_t raw;
  if (!ReadAdc(input, raw)) return false;
  current_ma = DecodeCurrent(raw);
  return true;
}

bool Axp517::GetBatteryAverageCurrent(float& current_ma) {
  return ReadSelectedCurrent(AdcInput::kBatteryAverageCurrent, current_ma);
}

bool Axp517::GetChargingCurrent(float& current_ma) {
  return ReadSelectedCurrent(AdcInput::kChargeCurrent, current_ma);
}

bool Axp517::GetDischargingCurrent(float& current_ma) {
  return ReadSelectedCurrent(AdcInput::kDischargeCurrent, current_ma);
}

bool Axp517::GetTsVoltage(float& voltage_mv) {
  uint16_t raw;
  if (!ReadBigEndian16(Register::kTsH, raw)) return false;
  voltage_mv = (raw & 0x3FFF) / 2.0f;
  return true;
}

bool Axp517::GetVbusVoltage(uint16_t& voltage_mv) {
  uint16_t raw;
  if (!ReadBigEndian16(Register::kVbusH, raw)) return false;
  voltage_mv = (raw & 0x3FFF) * 2;
  return true;
}

bool Axp517::GetVbusCurrent(float& current_ma) {
  uint16_t raw;
  if (!ReadBigEndian16(Register::kIbusH, raw)) return false;
  current_ma = raw & 0x3FFF;
  return true;
}

bool Axp517::GetSystemVoltage(uint16_t& voltage_mv) {
  uint16_t raw;
  if (!ReadAdc(AdcInput::kSystemVoltage, raw)) return false;
  voltage_mv = raw & 0x3FFF;
  return true;
}

bool Axp517::GetDieTemperature(float& celsius, DieTemperatureModel model) {
  uint16_t raw;
  if (!ReadAdc(AdcInput::kDieTemperature, raw)) return false;
  // 官方 USB 驱动与 V1 手册的拼接/公式不同，不混用两种算法。
  switch (model) {
    case DieTemperatureModel::kOfficialDriver: {
      const uint16_t packed = (((raw >> 8) & 0x3F) << 4) | (raw & 0x0F);
      const int decicelsius = (3605 - (packed & 0x0FFF)) * 100 / 191 / 5;
      celsius = decicelsius / 10.0f;
      return true;
    }
    case DieTemperatureModel::kDatasheetV1:
      celsius = (3552.0f - (raw & 0x3FFF)) / 1.79f + 25.0f;
      return true;
  }
  return false;
}

bool Axp517::ConfigurePowerKey(const PowerKeyConfig& config) {
  if (config.on_time_ms > 2000 || config.long_press_ms < 1000 ||
      config.long_press_ms > 2500 || config.off_time_ms < 4000 ||
      config.off_time_ms > 10000 ||
      (config.off_enabled && config.irq_wakeup)) {
    return false;
  }
  const uint8_t on = config.on_time_ms <= 128
                         ? 0
                         : config.on_time_ms <= 512
                               ? 1
                               : config.on_time_ms <= 1000 ? 2 : 3;
  // 官方 power_key 驱动的移位与手册不符，按 REG1C 的实际位域编码。
  const uint8_t value = (config.off_enabled ? 0x80 : 0) |
                        (config.irq_wakeup ? 0x40 : 0) |
                        (((config.long_press_ms - 1000) / 500) << 4) |
                        (((config.off_time_ms - 4000) / 2000) << 2) | on;
  return WriteRegister(static_cast<uint8_t>(Register::kPokSet), value);
}

bool Axp517::ConfigureNtc(const NtcConfig& config) {
  for (size_t i = 0; i < config.voltage_mv.size(); ++i) {
    if (config.voltage_mv[i] == 0 || config.voltage_mv[i] > 8191 ||
        (i != 0 &&
         config.voltage_mv[i - 1] <= config.voltage_mv[i])) return false;
  }
  if (config.charge_cold_mv > 8160 || config.work_cold_mv > 8160 ||
      config.charge_hot_mv > 510 || config.work_hot_mv > 510 ||
      config.charge_cold_mv <= config.charge_hot_mv ||
      config.work_cold_mv < config.charge_cold_mv ||
      config.work_hot_mv > config.charge_hot_mv ||
      config.cold_hysteresis_mv > 4080 || config.hot_hysteresis_mv > 1020 ||
      (config.current_ua != 20 && config.current_ua != 40 &&
       config.current_ua != 50 && config.current_ua != 60)) return false;
  const uint8_t current = config.current_ua == 20
                              ? 0
                              : config.current_ua == 40
                                    ? 1
                                    : config.current_ua == 50 ? 2 : 3;
  ntc_configured_ = false;
  if (!UpdateRegisterBits(Register::kAdcControl, 0x10, 0x10) ||
      !SetAdcChannelEnable(AdcChannel::kTs, true) ||
      !UpdateRegisterBits(Register::kTsSource, 0x08, 0) ||
      !WriteRegister(static_cast<uint8_t>(Register::kVltfChg), config.charge_cold_mv / 32) ||
      !WriteRegister(static_cast<uint8_t>(Register::kVhtfChg), config.charge_hot_mv / 2) ||
      !WriteRegister(static_cast<uint8_t>(Register::kVltfWork), config.work_cold_mv / 32) ||
      !WriteRegister(static_cast<uint8_t>(Register::kVhtfWork), config.work_hot_mv / 2) ||
      !WriteRegister(static_cast<uint8_t>(Register::kTsHysl2H), config.cold_hysteresis_mv / 16) ||
      !WriteRegister(static_cast<uint8_t>(Register::kTsHysh2L), config.hot_hysteresis_mv / 4) ||
      !UpdateRegisterBits(Register::kTsCfg, 0x1F, 0x04 | current)) return false;
  ntc_config_ = config;
  ntc_configured_ = true;
  return true;
}

bool Axp517::SetNtcEnable(bool enable) {
  if (enable && !ntc_configured_) return false;
  if (!enable && !SetJeitaEnable(false)) return false;
  return UpdateRegisterBits(Register::kTsCfg, 0x10, enable ? 0 : 0x10);
}

bool Axp517::SetTsCalibration(bool use_fixed_data, uint16_t raw) {
  if (raw > 0x3FFF) return false;
  return UpdateRegisterBits(Register::kTsCfgDataH, 0x3F, raw >> 8) &&
         WriteRegister(static_cast<uint8_t>(Register::kTsCfgDataL), raw & 0xFF) &&
         UpdateRegisterBits(Register::kTsSource, 0x08,
                            use_fixed_data ? 0x08 : 0);
}

bool Axp517::ConfigureJeita(const JeitaConfig& config) {
  if (config.cool_mv > 4080 || config.warm_mv > 2040 ||
      config.cool_mv <= config.warm_mv || config.cool_current_reduction > 3 ||
      config.warm_current_reduction > 3 || config.cool_voltage_reduction > 3 ||
      config.warm_voltage_reduction > 3) return false;
  const uint8_t value =
      (config.warm_current_reduction << 6) |
      (config.cool_current_reduction << 4) |
      (config.warm_voltage_reduction << 2) | config.cool_voltage_reduction;
  return WriteRegister(static_cast<uint8_t>(Register::kJeitaCool), config.cool_mv / 16) &&
         WriteRegister(static_cast<uint8_t>(Register::kJeitaWarm), config.warm_mv / 8) &&
         WriteRegister(static_cast<uint8_t>(Register::kJeitaCvCfg), value);
}

bool Axp517::SetJeitaEnable(bool enable) {
  if (enable) {
    uint8_t ts;
    if (!ntc_configured_ || !ReadRegister(static_cast<uint8_t>(Register::kTsCfg), &ts) ||
        (ts & 0x10)) {
      return false;
    }
  }
  return UpdateRegisterBits(Register::kJeitaCfg, 0x01, enable ? 0x01 : 0);
}

bool Axp517::ReadCompensatedTs(float& voltage_mv) {
  uint16_t raw, offset;
  uint8_t charge;
  if (!ReadBigEndian16(Register::kTsH, raw)) return false;
  int value = raw & 0x3FFF;
  if (ntc_config_.compensate_offset) {
    if (!ReadRegister(static_cast<uint8_t>(Register::kStatus1), &charge) ||
        !ReadAdc(AdcInput::kTsAverage, offset)) return false;
    offset &= 0x3FFF;
    const bool charging = (charge & 0x07) > 0 && (charge & 0x07) < 4;
    value += charging ? -static_cast<int>(offset)
                      : std::min(500, 0x3FFF - offset);
  }
  // 避免官方无符号减法在偏移大于 TS 时下溢。
  voltage_mv = std::max(0, std::min(0x3FFF, value)) / 2.0f;
  return true;
}

float Axp517::ConvertNtc(const NtcConfig& config, float voltage_mv) {
  if (voltage_mv >= config.voltage_mv.front()) return kNtcTemperatures[0];
  if (voltage_mv <= config.voltage_mv.back()) return kNtcTemperatures[15];
  for (size_t i = 1; i < config.voltage_mv.size(); ++i) {
    if (voltage_mv >= config.voltage_mv[i]) {
      const float fraction =
          (config.voltage_mv[i - 1] - voltage_mv) /
          (config.voltage_mv[i - 1] - config.voltage_mv[i]);
      return kNtcTemperatures[i - 1] +
             fraction * (kNtcTemperatures[i] - kNtcTemperatures[i - 1]);
    }
  }
  return kNtcTemperatures[15];
}

bool Axp517::GetBatteryTemperature(float& celsius) {
  if (!ntc_configured_) return false;
  uint8_t ts;
  Status status;
  if (!GetStatus(status) || !status.battery_present ||
      !ReadRegister(static_cast<uint8_t>(Register::kTsCfg), &ts) || (ts & 0x10)) {
    return false;
  }
  float voltage;
  if (!ReadCompensatedTs(voltage)) return false;
  float previous = ConvertNtc(ntc_config_, voltage);
  // 官方最多追加 10 次采样，要求连续两次温差不超过 10 ℃。
  for (int i = 0; i <= 10; ++i) {
    if (!ReadCompensatedTs(voltage)) return false;
    const float current = ConvertNtc(ntc_config_, voltage);
    if (std::fabs(previous - current) <= 10.0f) {
      celsius = current;
      return true;
    }
    previous = current;
  }
  return false;
}

bool Axp517::GetGaugeSoc(uint8_t& percent) {
  uint8_t value;
  Status status;
  if (!GetStatus(status)) return false;
  if (!status.battery_present) {
    percent = 0;
    return true;
  }
  if (!ReadRegister(static_cast<uint8_t>(Register::kGaugeSoc), &value)) return false;
  value &= 0x7F;
  if (value > 100) return false;
  percent = value;
  return true;
}

bool Axp517::GetBatteryLevel(uint8_t& percent) {
  Status status;
  uint8_t buffered;
  if (!GetStatus(status)) return false;
  if (!status.battery_present) {
    percent = 0;
    return true;
  }
  if (!ReadRegister(static_cast<uint8_t>(Register::kDataBuff), &buffered)) return false;
  if (buffered & 0x80) {
    if ((buffered & 0x7F) > 100) return false;
    percent = buffered & 0x7F;
    return true;
  }
  return GetGaugeSoc(percent);
}

bool Axp517::GetBatteryHealth(BatteryHealth& health) {
  Status status;
  FaultStatus fault;
  uint64_t irq;
  if (!GetStatus(status)) return false;
  if (!status.battery_present) {
    health = BatteryHealth::kUnknown;
    return true;
  }
  if (!GetFaultStatus(fault) || !GetIrqStatus(irq)) return false;
  // 官方用 IRQ2 bit2 判断 DEAD，但该位实际为结温过高，不误报电池报废。
  if (irq & static_cast<uint64_t>(Irq::kBatteryOvervoltage)) {
    health = BatteryHealth::kOvervoltage;
  } else if (irq & static_cast<uint64_t>(Irq::kChargeTimerExpired)) {
    health = BatteryHealth::kSafetyTimerExpired;
  } else if ((irq & static_cast<uint64_t>(Irq::kDieOvertemperature)) ||
             fault.ntc == NtcFault::kHotCharge ||
             fault.ntc == NtcFault::kHotWork) {
    health = BatteryHealth::kOverheat;
  } else if (fault.ntc == NtcFault::kColdCharge ||
             fault.ntc == NtcFault::kColdWork) {
    health = BatteryHealth::kCold;
  } else {
    health = fault.ntc == NtcFault::kNormal ? BatteryHealth::kGood
                                            : BatteryHealth::kUnknown;
  }
  return true;
}

bool Axp517::GetBatterySoh(uint8_t& percent) {
  uint8_t value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kGaugeSoh), &value) || value > 100) return false;
  percent = value;
  return true;
}

bool Axp517::GetCapacityLevel(CapacityLevel& level) {
  uint8_t soc, warning, shutdown;
  if (!GetBatteryLevel(soc) || !GetGaugeThresholds(warning, shutdown)) {
    return false;
  }
  level = soc == 100
              ? CapacityLevel::kFull
              : soc > 80
                    ? CapacityLevel::kHigh
                    : soc > warning
                          ? CapacityLevel::kNormal
                          : soc > shutdown ? CapacityLevel::kLow
                                           : CapacityLevel::kCritical;
  return true;
}

bool Axp517::SetGaugeThresholds(uint8_t warning_percent,
                                uint8_t shutdown_percent) {
  if (warning_percent < 5 || warning_percent > 20 || shutdown_percent > 15 ||
      shutdown_percent > warning_percent) return false;
  return WriteRegister(static_cast<uint8_t>(Register::kGaugeThld),
                       ((warning_percent - 5) << 4) | shutdown_percent);
}

bool Axp517::GetGaugeThresholds(uint8_t& warning_percent,
                                uint8_t& shutdown_percent) {
  uint8_t value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kGaugeThld), &value)) return false;
  warning_percent = (value >> 4) + 5;
  shutdown_percent = value & 0x0F;
  return true;
}

bool Axp517::GetCycleCount(uint16_t& cycles) {
  return ReadBigEndian16(Register::kCycleH, cycles);
}

bool Axp517::GetTimeToEmptyRaw(uint16_t& value) {
  return ReadBigEndian16(Register::kGaugeTime2EmptyH, value);
}

bool Axp517::GetTimeToFullRaw(uint16_t& value) {
  return ReadBigEndian16(Register::kGaugeTime2FullH, value);
}

bool Axp517::SetGaugeEnable(bool enable) {
  return UpdateRegisterBits(Register::kClkEn, 0x04, enable ? 0x04 : 0);
}

bool Axp517::IsBatteryModelUpdated(bool& updated) {
  uint8_t value;
  if (!ReadRegister(static_cast<uint8_t>(Register::kGaugeConfig), &value)) return false;
  updated = (value & kModelUpdated) != 0;
  return true;
}

bool Axp517::RestartBrom() {
  return UpdateRegisterBits(Register::kGaugeConfig, kBromEnable, 0) &&
         UpdateRegisterBits(Register::kGaugeConfig, kBromEnable,
                            kBromEnable);
}

bool Axp517::UpdateBatteryModel(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 ||
      length > kMaxBatteryModelSize) {
    return false;
  }
  Status status;
  if (!GetStatus(status) || !status.battery_present) return false;
  // 先撤销有效标记，任何写入或校验失败都不能留下“模型已更新”。
  bool success =
      UpdateRegisterBits(Register::kGaugeConfig, kModelUpdated, 0) &&
      RestartBrom();
  for (size_t i = 0; success && i < length; ++i) {
    success = WriteRegister(static_cast<uint8_t>(Register::kGaugeBrom), data[i]);
  }
  if (success) success = RestartBrom();
  for (size_t i = 0; success && i < length; ++i) {
    uint8_t actual;
    success = ReadRegister(static_cast<uint8_t>(Register::kGaugeBrom), &actual) && actual == data[i];
  }
  const bool closed =
      UpdateRegisterBits(Register::kGaugeConfig, kBromEnable, 0);
  success = success && closed;
  if (success) {
    success = UpdateRegisterBits(Register::kGaugeConfig, kModelUpdated,
                                 kModelUpdated);
  }
  // 即使失败也尝试退出 BROM 并复位；复位恢复原有充电/Buck 配置。
  const bool reset = ResetGauge();
  if (!success || !reset) {
    UpdateRegisterBits(Register::kGaugeConfig,
                       kBromEnable | kModelUpdated, 0);
    return false;
  }
  return true;
}

bool Axp517::ApplyBatteryModelIfNeeded(const uint8_t* data, size_t length,
                                       bool& updated) {
  if (data == nullptr || length == 0 ||
      length > kMaxBatteryModelSize) {
    return false;
  }
  bool valid;
  if (!IsBatteryModelUpdated(valid)) return false;
  if (!valid && !UpdateBatteryModel(data, length)) return false;
  updated = !valid;
  return true;
}

bool Axp517::ResetGauge() {
  uint8_t module, current;
  if (!ReadRegister(static_cast<uint8_t>(Register::kModuleEn), &module) ||
      !ReadRegister(static_cast<uint8_t>(Register::kIccCfg), &current)) {
    return false;
  }
  bool success = SetChargeEnable(false);
  if (success) {
    DelayMs(500);
    success = UpdateRegisterBits(Register::kResetCfg, kMcuReset,
                                 kMcuReset);
    const bool released =
        UpdateRegisterBits(Register::kResetCfg, kMcuReset, 0);
    success = success && released;
    DelayMs(500);
  }
  const bool current_restored =
      UpdateRegisterBits(Register::kIccCfg, 0x7F, current);
  const bool module_restored =
      UpdateRegisterBits(Register::kModuleEn, 0x0A, module);
  return success && current_restored && module_restored;
}

bool Axp517::ReadGaugeWord(uint8_t address, uint16_t& value,
                           uint32_t settle_ms) {
  if (!WriteRegister(static_cast<uint8_t>(Register::kFgAddr), address)) return false;
  DelayMs(settle_ms);
  return ReadBigEndian16(Register::kFgDataH, value);
}

bool Axp517::CheckGauge(GaugeDiagnostics& diagnostics, bool recover) {
  GaugeDiagnostics result;
  uint16_t calculated;
  if (!GetGaugeSoc(result.soc_percent) || !ReadGaugeWord(0x80, calculated) ||
      !ReadGaugeWord(0x8E, result.pct_now)) return false;
  result.calculated_soc_percent = (calculated >> 8) & 0x7F;
  result.reset_recommended =
      std::abs(static_cast<int>(result.soc_percent) -
               result.calculated_soc_percent) >= 2 ||
      (result.pct_now & 0x8000) != 0 || result.pct_now > 400;
  if (recover && result.reset_recommended && !ResetGauge()) return false;
  diagnostics = result;
  return true;
}

bool Axp517::StartSocSmoothing(uint8_t percent) {
  return percent <= 100 && WriteRegister(static_cast<uint8_t>(Register::kDataBuff), 0x80 | percent);
}

bool Axp517::StepSocSmoothing(bool& active) {
  uint8_t value, soc;
  Status status;
  if (!ReadRegister(static_cast<uint8_t>(Register::kDataBuff), &value) || !GetStatus(status)) {
    return false;
  }
  if ((value & 0x80) == 0 || !status.battery_present || (value & 0x7F) > 100) {
    if (!StopSocSmoothing()) return false;
    active = false;
    return true;
  }
  if (!GetGaugeSoc(soc)) return false;
  uint8_t displayed = value & 0x7F;
  if (soc <= displayed) {
    if (!StopSocSmoothing()) return false;
    active = false;
    return true;
  }
  if (status.vbus_good && !StartSocSmoothing(displayed + 1)) return false;
  active = true;
  return true;
}

bool Axp517::StopSocSmoothing() {
  return WriteRegister(static_cast<uint8_t>(Register::kDataBuff), 0);
}

bool Axp517::EstimateCapacity(uint16_t design_capacity_mah, uint16_t cycle_life,
                             uint8_t lifetime_loss_percent,
                             uint32_t& remaining_uah, uint32_t& full_uah) {
  if (design_capacity_mah == 0 || cycle_life == 0 ||
      lifetime_loss_percent > 100) {
    return false;
  }
  uint16_t cycles;
  uint8_t soc;
  if (!GetCycleCount(cycles) || !GetBatteryLevel(soc)) return false;
  const uint64_t design = static_cast<uint64_t>(design_capacity_mah) * 1000;
  const uint64_t loss =
      design * lifetime_loss_percent * cycles / (100ULL * cycle_life);
  full_uah = static_cast<uint32_t>(design - std::min(design, loss));
  remaining_uah = static_cast<uint32_t>(design * soc / 100);
  return true;
}

bool Axp517::GetIrqStatus(uint64_t& status) {
  uint8_t data[5];
  if (!ReadRegister(static_cast<uint8_t>(Register::kIrq0), data, sizeof(data))) return false;
  uint64_t result = 0;
  for (size_t i = 0; i < sizeof(data); ++i) {
    result |= static_cast<uint64_t>(data[i]) << (8 * i);
  }
  status = result & kValidIrqs;
  return true;
}

bool Axp517::GetInterruptStatus(InterruptStatus& status) {
  InterruptStatus result;
  if (!GetIrqStatus(result.power) || !GetPdAlerts(result.pd)) return false;
  status = result;
  return true;
}

bool Axp517::SetIrqEnable(uint64_t mask, bool enable) {
  if ((mask & ~kValidIrqs) != 0) return false;
  for (size_t i = 0; i < 5; ++i) {
    const uint8_t byte = (mask >> (8 * i)) & 0xFF;
    const Register reg =
        static_cast<Register>(static_cast<uint8_t>(Register::kIrqEn0) + i);
    if (byte != 0 && !UpdateRegisterBits(reg, byte, enable ? byte : 0)) {
      return false;
    }
  }
  return true;
}

bool Axp517::GetIrqEnable(uint64_t& mask) {
  uint8_t data[5];
  if (!ReadRegister(static_cast<uint8_t>(Register::kIrqEn0), data, sizeof(data))) return false;
  uint64_t result = 0;
  for (size_t i = 0; i < sizeof(data); ++i) {
    result |= static_cast<uint64_t>(data[i]) << (8 * i);
  }
  mask = result & kValidIrqs;
  return true;
}

bool Axp517::ClearIrqs(uint64_t mask) {
  if ((mask & ~kValidIrqs) != 0) return false;
  for (size_t i = 0; i < 5; ++i) {
    const uint8_t byte = (mask >> (8 * i)) & 0xFF;
    const Register reg =
        static_cast<Register>(static_cast<uint8_t>(Register::kIrq0) + i);
    if (byte != 0 && !WriteRegister(static_cast<uint8_t>(reg), byte)) {
      return false;
    }
  }
  return true;
}

bool Axp517::ClearAllIrqs() {
  return ClearIrqs(kValidIrqs);
}

bool Axp517::GetPdAlerts(uint16_t& alerts) {
  return ReadLittleEndian16(Register::kIrqPdAlertlStatus, alerts);
}

bool Axp517::SetPdAlertMask(uint16_t mask) {
  return (mask & ~kValidPdAlerts) == 0 &&
         WriteLittleEndian16(Register::kTcpcAlertMask, mask);
}

bool Axp517::ClearPdAlerts(uint16_t mask) {
  return (mask & ~kValidPdAlerts) == 0 &&
         WriteLittleEndian16(Register::kIrqPdAlertlStatus, mask);
}

bool Axp517::GetTcpcIdentity(TcpcIdentity& identity) {
  TcpcIdentity result;
  if (!ReadLittleEndian16(Register::kTcpcVendorId, result.vendor_id) ||
      !ReadLittleEndian16(Register::kTcpcProductId, result.product_id) ||
      !ReadLittleEndian16(Register::kTcpcBcdDev, result.device_revision) ||
      !ReadLittleEndian16(Register::kTcpcTcRev, result.typec_revision) ||
      !ReadLittleEndian16(Register::kTcpcPdRev, result.pd_revision) ||
      !ReadLittleEndian16(Register::kTcpcPdIntRev, result.interface_revision)) {
    return false;
  }
  identity = result;
  return true;
}

bool Axp517::InitTypec(bool enable_pd_irqs, bool self_powered) {
  uint16_t vendor;
  Status status;
  if (!ReadLittleEndian16(Register::kTcpcVendorId, vendor) ||
      vendor != kVendorId ||
      !GetStatus(status) || !status.system_on || !SetTypecEnable(true) ||
      !ResetTypec()) return false;
  self_powered_ = self_powered;
  bool ready = false;
  for (int i = 0; i <= 200; ++i) {
    uint8_t power;
    if (!ReadRegister(static_cast<uint8_t>(Register::kTcpcPowerStatus), &power)) return false;
    if ((power & 0x40) == 0) { ready = true; break; }
    if (i < 200) DelayMs(10);
  }
  if (!ready) return false;
  // FAULT_STATUS 仅一个字节，不能照抄官方 write16 误写扩展状态寄存器。
  if (!ClearTcpcFaults(0x80) || !SetPdAlertMask(0) ||
      !SetCcTerminations(CcTermination::kRd, CcTermination::kRd) ||
      !UpdateRegisterBits(Register::kPdState, 0x08, 0x08) ||
      !UpdateRegisterBits(Register::kTwiAddrStatic, 0x01, 0x01) ||
      !ClearPdAlerts(kValidPdAlerts) ||
      !SetTcpcStatusMasks(0x04, 0, 0x01, 0) ||
      !SetPdReceiveMask(0) || !SetPdMessageHeader(false, false) ||
      !SetVbusDetectEnable(true)) return false;
  // CC/电源、TX 结果、RX/硬复位、故障、溢出及 vSafe0V。
  return SetPdAlertMask(enable_pd_irqs ? 0x267F : 0);
}

bool Axp517::SetTypecEnable(bool enable) {
  if (!UpdateRegisterBits(Register::kClkEn, 0x08, enable ? 0x08 : 0)) {
    return false;
  }
  if (enable) DelayMs(20);
  return true;
}

bool Axp517::ResetTypec() {
  if (!UpdateRegisterBits(Register::kCcGeneralControl, 0x20, 0x20)) {
    return false;
  }
  return UpdateRegisterBits(Register::kCcGeneralControl, 0x20, 0);
}

bool Axp517::SetCcTerminations(CcTermination cc1, CcTermination cc2,
                               RpCurrent current, bool dual_role) {
  if (static_cast<uint8_t>(cc1) > 3 || static_cast<uint8_t>(cc2) > 3 ||
      static_cast<uint8_t>(current) > 2) return false;
  if (!self_powered_ &&
      (dual_role || cc1 != CcTermination::kRd ||
       cc2 != CcTermination::kRd)) {
    return false;
  }
  const uint8_t value =
      (dual_role ? 0x40 : 0) | (static_cast<uint8_t>(current) << 4) |
      (static_cast<uint8_t>(cc2) << 2) | static_cast<uint8_t>(cc1);
  return UpdateRegisterBits(Register::kTcpcRoleCtrl, 0x7F, value);
}

bool Axp517::SetCc(CcTermination termination, RpCurrent current) {
  uint8_t power, control;
  if (!ReadRegister(static_cast<uint8_t>(Register::kTcpcPowerStatus), &power)) return false;
  if ((power & 0x02) == 0) {
    return SetCcTerminations(termination, termination, current);
  }
  if (!ReadRegister(static_cast<uint8_t>(Register::kTcpcCtrl), &control)) return false;
  return (control & 0x01)
             ? SetCcTerminations(CcTermination::kOpen, termination, current)
             : SetCcTerminations(termination, CcTermination::kOpen, current);
}

bool Axp517::SetTypecRole(TypecRole role, RpCurrent current) {
  if (static_cast<uint8_t>(role) > 2) return false;
  const bool source = role == TypecRole::kSource;
  const CcTermination termination =
      source ? CcTermination::kRp : CcTermination::kRd;
  if (!SetCcTerminations(termination, termination, current,
                         role == TypecRole::kDualRole) ||
      !SetPdMessageHeader(source, source)) return false;
  return SendTcpcCommand(TcpcCommand::kLookForConnection);
}

Axp517::CcState Axp517::DecodeCc(uint8_t raw, bool sink) {
  if (raw == 1) return sink ? CcState::kRpDefault : CcState::kRa;
  if (raw == 2) return sink ? CcState::kRp1500Ma : CcState::kRd;
  if (raw == 3 && sink) return CcState::kRp3000Ma;
  return CcState::kOpen;
}

bool Axp517::IsRp(CcState state) {
  return state == CcState::kRpDefault || state == CcState::kRp1500Ma ||
         state == CcState::kRp3000Ma;
}

bool Axp517::GetCcStatus(CcStatus& status) {
  uint8_t role, cc;
  if (!ReadRegister(static_cast<uint8_t>(Register::kTcpcRoleCtrl), &role) ||
      !ReadRegister(static_cast<uint8_t>(Register::kTcpcCcStatus), &cc)) {
    return false;
  }
  CcStatus result;
  result.looking_for_connection = (cc & 0x20) != 0;
  // 搜索期间 CC 读数不能当作已经连接。
  if (!result.looking_for_connection) {
    const bool drp = (role & 0x40) != 0;
    result.cc1 = DecodeCc(cc & 0x03,
                          (cc & 0x10) || (!drp && (role & 0x03) == 2));
    result.cc2 = DecodeCc((cc >> 2) & 0x03,
                          (cc & 0x10) || (!drp && ((role >> 2) & 0x03) == 2));
    result.sink_attached = IsRp(result.cc1) || IsRp(result.cc2);
    result.source_attached =
        (result.cc1 == CcState::kRd) != (result.cc2 == CcState::kRd);
    result.audio_accessory =
        result.cc1 == CcState::kRa && result.cc2 == CcState::kRa;
    result.debug_accessory =
        (result.cc1 == CcState::kRd && result.cc2 == CcState::kRd) ||
        (IsRp(result.cc1) && IsRp(result.cc2));
  }
  status = result;
  return true;
}

bool Axp517::SetPolarity(Polarity polarity) {
  if (polarity != Polarity::kCc1 && polarity != Polarity::kCc2) return false;
  uint8_t role;
  CcStatus cc;
  if (!ReadRegister(static_cast<uint8_t>(Register::kTcpcRoleCtrl), &role) || !GetCcStatus(cc) ||
      cc.looking_for_connection) {
    return false;
  }
  const bool cc2 = polarity == Polarity::kCc2;
  if (role & 0x40) {
    role &= ~0x40;
    const uint8_t shift = cc2 ? 2 : 0;
    const uint8_t termination = (cc2 ? cc.cc2 : cc.cc1) == CcState::kRd ? 1 : 2;
    role = (role & ~(0x03 << shift)) | (termination << shift);
  }
  if (self_powered_) role |= cc2 ? 0x03 : 0x0C;
  return UpdateRegisterBits(Register::kTcpcRoleCtrl, 0x7F, role) &&
         UpdateRegisterBits(Register::kTcpcCtrl, 0x01, cc2 ? 0x01 : 0);
}

bool Axp517::ApplyCcResistance(Polarity polarity) {
  if (polarity != Polarity::kCc1 && polarity != Polarity::kCc2) return false;
  if (!self_powered_) return true;
  uint8_t role;
  if (!ReadRegister(static_cast<uint8_t>(Register::kTcpcRoleCtrl), &role)) return false;
  if ((role & 0x03) != ((role >> 2) & 0x03)) return true;
  const uint8_t mask = polarity == Polarity::kCc1 ? 0x0C : 0x03;
  return UpdateRegisterBits(Register::kTcpcRoleCtrl, mask, mask);
}

bool Axp517::SetVconnEnable(bool enable) {
  return UpdateRegisterBits(Register::kTcpcPowerCtrl, 0x01, enable ? 0x01 : 0);
}

bool Axp517::SetVbusDetectEnable(bool enable) {
  return SendTcpcCommand(enable ? TcpcCommand::kEnableVbusDetect
                                : TcpcCommand::kDisableVbusDetect);
}

bool Axp517::SetVbusPath(bool source, bool sink) {
  if ((source && sink) || (source && !self_powered_)) return false;
  if (!source &&
      !SendTcpcCommand(TcpcCommand::kDisableSourceVbus)) {
    return false;
  }
  if (!sink && !SendTcpcCommand(TcpcCommand::kDisableSinkVbus)) return false;
  if (source && !SendTcpcCommand(TcpcCommand::kSourceVbusDefault)) return false;
  if (sink && !SendTcpcCommand(TcpcCommand::kSinkVbus)) return false;
  return true;
}

bool Axp517::GetTcpcStatus(TcpcStatus& status) {
  uint8_t data[4];
  if (!ReadRegister(static_cast<uint8_t>(Register::kTcpcPowerStatus), data, sizeof(data))) return false;
  TcpcStatus result;
  result.power = data[0];
  result.fault = data[1];
  result.extended_status = data[2];
  result.extended_alert = data[3];
  result.vbus_present = (data[0] & 0x04) != 0;
  result.sourcing_vbus = (data[0] & 0x10) != 0;
  result.sinking_vbus = (data[0] & 0x01) != 0;
  result.vconn_present = (data[0] & 0x02) != 0;
  result.vbus_safe0v = (data[2] & 0x01) != 0;
  status = result;
  return true;
}

bool Axp517::ClearTcpcFaults(uint8_t mask) {
  return WriteRegister(static_cast<uint8_t>(Register::kTcpcFaultStatus), mask);
}

bool Axp517::SetTcpcStatusMasks(uint8_t power, uint8_t fault,
                                uint8_t extended_status,
                                uint8_t extended_alert) {
  return WriteRegister(static_cast<uint8_t>(Register::kTcpcPowerStatusMask), power) &&
         WriteRegister(static_cast<uint8_t>(Register::kTcpcFaultStatusMask), fault) &&
         WriteRegister(static_cast<uint8_t>(Register::kTcpcExtendedStatusMask), extended_status) &&
         WriteRegister(static_cast<uint8_t>(Register::kTcpcAlertExtendedMask), extended_alert);
}

bool Axp517::ClearTcpcExtendedAlerts(uint8_t mask) {
  return WriteRegister(static_cast<uint8_t>(Register::kTcpcAlertExtended), mask);
}

bool Axp517::SendTcpcCommand(TcpcCommand command) {
  switch (command) {
    case TcpcCommand::kWakeI2c:
    case TcpcCommand::kDisableVbusDetect:
    case TcpcCommand::kEnableVbusDetect:
    case TcpcCommand::kDisableSinkVbus:
    case TcpcCommand::kSinkVbus:
    case TcpcCommand::kDisableSourceVbus:
    case TcpcCommand::kSourceVbusDefault:
    case TcpcCommand::kSourceVbusHigh:
    case TcpcCommand::kLookForConnection:
    case TcpcCommand::kReceiveOneMore:
    case TcpcCommand::kI2cIdle:
      return WriteRegister(static_cast<uint8_t>(Register::kTcpcCommand),
                           static_cast<uint8_t>(command));
  }
  return false;
}

bool Axp517::SetPdMessageHeader(bool source, bool host, PdRevision revision) {
  if (static_cast<uint8_t>(revision) > 2) return false;
  return UpdateRegisterBits(Register::kTcpcMsgHdrInfo, 0x0F,
                            (source ? 0x01 : 0) | (host ? 0x08 : 0) |
                            (static_cast<uint8_t>(revision) << 1));
}

bool Axp517::SetPdReceiveMask(uint8_t mask) {
  // 官方默认不接收 Hard Reset，调用方可在接入策略层后显式打开 bit5。
  return (mask & 0xC0) == 0 && WriteRegister(static_cast<uint8_t>(Register::kTcpcRxDetect), mask);
}

bool Axp517::ReceivePdMessage(PdMessage& message) {
  uint16_t alerts;
  if (!GetPdAlerts(alerts) || (alerts & kRxMessageAlert) == 0) return false;
  uint8_t prefix[2];
  if (!ReadNoIncrement(Register::kTcpcRxByteCnt, prefix, sizeof(prefix))) {
    return false;
  }
  // count 包含帧类型、2 字节消息头和数据，不含 count 字节自身。
  if (prefix[0] < 3 || prefix[0] > 31 || prefix[1] > 4 ||
      ((prefix[0] - 3) % 4) != 0) return false;
  std::array<uint8_t, 32> data{};
  // 固定地址 FIFO：重新发起读取从 count 开始，绝不能读取 DA+1 等地址。
  if (!ReadNoIncrement(Register::kTcpcRxByteCnt, data.data(), prefix[0] + 1) ||
      data[0] != prefix[0] || data[1] != prefix[1]) return false;
  PdMessage result;
  result.frame_type = static_cast<PdTransmitType>(data[1]);
  result.header = static_cast<uint16_t>(data[2]) |
                  (static_cast<uint16_t>(data[3]) << 8);
  result.data_object_count = (result.header >> 12) & 0x07;
  if (data[0] != 3 + 4 * result.data_object_count) return false;
  for (size_t i = 0; i < result.data_object_count; ++i) {
    const size_t offset = 4 + 4 * i;
    result.data_objects[i] = static_cast<uint32_t>(data[offset]) |
        (static_cast<uint32_t>(data[offset + 1]) << 8) |
        (static_cast<uint32_t>(data[offset + 2]) << 16) |
        (static_cast<uint32_t>(data[offset + 3]) << 24);
  }
  if (!ClearPdAlerts(kRxMessageAlert)) return false;
  message = result;
  return true;
}

bool Axp517::DiscardPdMessage() {
  return ClearPdAlerts(kRxMessageAlert | kRxOverflowAlert);
}

bool Axp517::TransmitPdMessage(PdTransmitType type, const PdMessage* message,
                               PdRevision revision) {
  const uint8_t frame = static_cast<uint8_t>(type);
  if (frame > 7 || static_cast<uint8_t>(revision) > 2) return false;
  if (frame < 5) {
    if (message == nullptr ||
        message->data_object_count > kMaxPdDataObjects ||
        message->data_object_count != ((message->header >> 12) & 0x07)) {
      return false;
    }
    std::array<uint8_t, 31> data{};
    data[0] = 2 + 4 * message->data_object_count;
    // data[1] 位于字节计数寄存器之后的 TX FIFO。
    data[1] = message->header & 0xFF;
    data[2] = message->header >> 8;
    for (size_t i = 0; i < message->data_object_count; ++i) {
      for (size_t j = 0; j < 4; ++j) {
        data[3 + 4 * i + j] =
            (message->data_objects[i] >> (8 * j)) & 0xFF;
      }
    }
    // 官方使用多次单字节写入固定地址 0xDC，不做自动地址递增。
    for (size_t i = 0; i <= data[0]; ++i) {
      if (!WriteRegister(static_cast<uint8_t>(Register::kTcpcTxByteCnt), data[i])) return false;
    }
  } else if (message != nullptr) {
    return false;
  }
  const uint8_t retries = revision == PdRevision::kRev30 ? 2 : 3;
  return WriteRegister(static_cast<uint8_t>(Register::kTcpcTransmit), (retries << 4) | frame);
}

bool Axp517::SetBistEnable(bool enable) {
  return UpdateRegisterBits(Register::kTcpcCtrl, 0x02, enable ? 0x02 : 0);
}

bool Axp517::SetVbusDischarge(bool automatic, bool bleed) {
  return UpdateRegisterBits(Register::kTcpcPowerCtrl, 0x18,
                            (automatic ? 0x10 : 0) | (bleed ? 0x08 : 0));
}

bool Axp517::SetAutoDischargeThreshold(bool pd_contract, bool pps,
                                       uint16_t requested_voltage_mv) {
  uint8_t control;
  if (!ReadRegister(static_cast<uint8_t>(Register::kTcpcPowerCtrl), &control)) return false;
  int threshold = 0;
  if (requested_voltage_mv != 0) {
    if ((control & 0x80) != 0 || !pd_contract) threshold = 3500;
    else {
      threshold = std::max(
          0, (95 * requested_voltage_mv / 100 - 750 - (pps ? 100 : 500)) *
                 90 / 100);
    }
  }
  if (threshold / 25 > 0x3FF) return false;
  return WriteLittleEndian16(Register::kTcpcVbusSinkDisconnectThresh,
                             threshold / 25);
}

bool Axp517::SetFastRoleSwapEnable(bool enable) {
  return WriteLittleEndian16(Register::kTcpcVbusSinkDisconnectThresh,
                             enable ? 0 : 0x8C) &&
         UpdateRegisterBits(Register::kTcpcPowerCtrl, 0x80, enable ? 0x80 : 0);
}

bool Axp517::SetVbusThresholds(uint16_t stop_discharge_mv,
                               uint16_t alarm_low_mv,
                               uint16_t alarm_high_mv) {
  if (stop_discharge_mv > 25575 || alarm_low_mv > 25575 ||
      alarm_high_mv > 25575 || alarm_low_mv > alarm_high_mv) {
    return false;
  }
  return WriteLittleEndian16(Register::kTcpcVbusStopDischargeThresh,
                             stop_discharge_mv / 25) &&
         WriteLittleEndian16(Register::kTcpcVbusVoltageAlarmLoCfg,
                             alarm_low_mv / 25) &&
         WriteLittleEndian16(Register::kTcpcVbusVoltageAlarmHiCfg,
                             alarm_high_mv / 25);
}

bool Axp517::GetTcpcVbusVoltage(uint16_t& voltage_mv) {
  uint16_t raw;
  if (!ReadLittleEndian16(Register::kTcpcVbusVoltage, raw)) return false;
  const uint32_t value = static_cast<uint32_t>(raw & 0x03FF) * 25;
  if (value > 65535) return false;
  voltage_mv = value;
  return true;
}

bool Axp517::EnterTypecLowPower() {
  CcStatus status;
  if (!GetCcStatus(status) || status.cc1 != CcState::kOpen ||
      status.cc2 != CcState::kOpen) {
    return false;
  }
  // AXP517 官方以 SOFT_AWAKE_EN=1 请求低功耗，命名虽为 awake，语义相反。
  return UpdateRegisterBits(Register::kPdState, 0x08, 0x08) &&
         UpdateRegisterBits(Register::kAwakeEn, 0x01, 0x01);
}

bool Axp517::WakeTypec() {
  return SendTcpcCommand(TcpcCommand::kWakeI2c) &&
         UpdateRegisterBits(Register::kAwakeEn, 0x01, 0) &&
         SetTypecEnable(true);
}

bool Axp517::NotifyTypecCharging(bool enable) {
  if (enable && !SetTypecEnable(true)) return false;
  if (!TransmitPdMessage(PdTransmitType::kHardReset, nullptr,
                         PdRevision::kRev30)) {
    return false;
  }
  if (!enable) return SetTypecEnable(false);
  return ResetTypec();
}

bool Axp517::ReadRegister(uint8_t reg, uint8_t* data, size_t length) {
  if (bus_ == nullptr || data == nullptr || length == 0 || length > 256) {
    return false;
  }
  if (bus_->WriteRead(&reg, 1, data, length)) return true;
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
             "AXP517 register read failed (register: 0x%02X)\n", reg);
  return false;
}

bool Axp517::ReadNoIncrement(Register reg, uint8_t* data, size_t length) {
  if (bus_ == nullptr || data == nullptr || length == 0 || length > 32) {
    return false;
  }
  const uint8_t address = static_cast<uint8_t>(reg);
  for (size_t i = 0; i < length; ++i) {
    if (!bus_->WriteRead(&address, 1, &data[i], 1)) return false;
  }
  return true;
}

bool Axp517::WriteBytes(Register reg, const uint8_t* data, size_t length) {
  if (bus_ == nullptr || data == nullptr || length == 0 || length > 32) {
    return false;
  }
  std::array<uint8_t, 33> packet{};
  packet[0] = static_cast<uint8_t>(reg);
  std::copy_n(data, length, packet.begin() + 1);
  if (bus_->Write(packet.data(), length + 1)) return true;
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
             "AXP517 register write failed (register: 0x%02X)\n", packet[0]);
  return false;
}

bool Axp517::SetCommonConfiguration(uint8_t value) {
  return UpdateRegisterBits(Register::kCommCfg, 0x7F, value);
}

bool Axp517::SetTcpcStandardConfiguration(uint8_t value) {
  return WriteRegister(static_cast<uint8_t>(Register::kTcpcConfigStdOutput), value);
}

bool Axp517::WriteRegister(uint8_t reg, uint8_t value) {
  return WriteBytes(static_cast<Register>(reg), &value, 1);
}

bool Axp517::UpdateRegisterBits(Register reg, uint8_t mask, uint8_t value) {
  uint8_t original;
  if (!ReadRegister(static_cast<uint8_t>(reg), &original)) return false;
  return WriteRegister(static_cast<uint8_t>(reg), (original & ~mask) | (value & mask));
}

bool Axp517::ReadBigEndian16(Register reg, uint16_t& value) {
  uint8_t data[2];
  if (!ReadRegister(static_cast<uint8_t>(reg), data, sizeof(data))) return false;
  value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  return true;
}

bool Axp517::ReadLittleEndian16(Register reg, uint16_t& value) {
  uint8_t data[2];
  if (!ReadRegister(static_cast<uint8_t>(reg), data, sizeof(data))) return false;
  value = static_cast<uint16_t>(data[0]) |
          (static_cast<uint16_t>(data[1]) << 8);
  return true;
}

bool Axp517::WriteLittleEndian16(Register reg, uint16_t value) {
  const uint8_t data[] = {static_cast<uint8_t>(value),
                          static_cast<uint8_t>(value >> 8)};
  return WriteBytes(reg, data, sizeof(data));
}

}  // namespace cpp_bus_driver
