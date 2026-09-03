/*
 * @Description: SGM41562 系列电池充电管理芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-09-03 18:00:00
 * @License: GPL 3.0
 */
#include "sgm41562xx.h"

namespace cpp_bus_driver {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
constexpr const uint8_t Sgm41562xx::kInitSequenceAb[];
constexpr const uint8_t Sgm41562xx::kInitSequenceS[];
#endif

namespace {
constexpr uint8_t kRegisterResetMask = 0x80;
constexpr uint8_t kChargeDisableMask = 0x08;
constexpr uint8_t kHighImpedanceEnableMask = 0x10;
constexpr uint8_t kBatteryUndervoltageMask = 0x07;
constexpr uint8_t kInputVoltageLimitMask = 0xF0;
constexpr uint8_t kInputCurrentLimitMask = 0x0F;
constexpr uint8_t kExtendedInputCurrentLimitMask = 0xF8;
constexpr uint8_t kFastChargeCurrentAbMask = 0x3F;
constexpr uint8_t kFastChargeCurrentSMask = 0x7F;
constexpr uint8_t kTerminationCurrentMask = 0x0F;
constexpr uint8_t kDischargeCurrentMask = 0xF0;
constexpr uint8_t kChargeVoltageAbMask = 0xFC;
constexpr uint8_t kChargeVoltageSMask = 0xFE;
constexpr uint8_t kSystemVoltageAbMask = 0x0F;
constexpr uint8_t kSystemVoltageSMask = 0x1F;
constexpr uint8_t kThermalRegulationAbMask = 0x30;
constexpr uint8_t kThermalRegulationSMask = 0x60;
constexpr uint8_t kExtendedTerminationMultiplierMask = 0x04;
constexpr uint8_t kExtendedPrechargeMultiplierMask = 0x08;
constexpr uint8_t kExtendedPrechargeCurrentMask = 0xF0;
constexpr uint8_t kPrechargeThresholdAbMask = 0x02;
constexpr uint8_t kPrechargeThresholdSMask = 0x02;
constexpr uint8_t kRechargeThresholdMask = 0x01;
constexpr uint8_t kWatchdogResetAbMask = 0x40;
constexpr uint8_t kWatchdogResetSMask = 0x01;
constexpr uint8_t kWatchdogInDischargeEnableMask = 0x80;
constexpr uint8_t kWatchdogMask = 0x60;
constexpr uint8_t kChargeTerminationEnableMask = 0x10;
constexpr uint8_t kSafetyTimerEnableMask = 0x08;
constexpr uint8_t kSafetyTimerSettingMask = 0x06;
constexpr uint8_t kTerminationTimerEnableMask = 0x01;
constexpr uint8_t kNtcEnableMask = 0x80;
constexpr uint8_t kSafetyTimerExtendedMask = 0x40;
constexpr uint8_t kPcbProtectionDisableAbMask = 0x80;
constexpr uint8_t kPcbProtectionDisableSMask = 0x40;
constexpr uint8_t kInputVoltageLoopDisableAbMask = 0x40;
constexpr uint8_t kInputVoltageLoopDisableSMask = 0x80;
constexpr uint8_t kInputCurrentLimitReleaseMask = 0x40;
constexpr uint8_t kInputCurrentLimitAdd200Mask = 0x20;
constexpr uint8_t kInputOvervoltageSelectMask = 0x20;
constexpr uint8_t kShippingModeEnableMask = 0x20;
constexpr uint8_t kShippingModeDelayMask = 0xC0;
constexpr uint8_t kPowerPathSwitchForceMask = 0x08;
constexpr uint8_t kBatteryPowerDisableMask = 0x04;
constexpr uint8_t kInputOvervoltageProtectionDisableMask = 0x02;
constexpr uint8_t kFineChargeCurrentScaleMask = 0x01;
constexpr uint8_t kSgm41562BChargeVoltageReset = 0xA3;
constexpr uint8_t kSgm41562BSystemVoltageReset = 0x37;
constexpr uint8_t kSgm41562SaChargeVoltageReset = 0x8D;
constexpr uint8_t kSgm41562SaSystemVoltageReset = 0x73;
constexpr uint8_t kSafetyTimerHours[] = {3, 5, 8, 12};

/**
 * @brief 检查数值是否位于寄存器范围内并符合步进
 * @param value 待检查数值
 * @param minimum 最小值
 * @param maximum 最大值
 * @param step 步进值
 * @return 数值有效返回true，否则返回false
 */
bool IsRegisterValueValid(
    uint16_t value, uint16_t minimum, uint16_t maximum, uint16_t step) {
  return value >= minimum && value <= maximum &&
         (value - minimum) % step == 0;
}
}  // namespace

bool Sgm41562xx::Init(int32_t freq_hz) {
  chip_type_ = ChipType::kUnknown;

  if (rst_ != kDefaultValue) {
    bool result = true;
    result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);
    result &= GpioWrite(rst_, 0);
    DelayMs(10);
    result &= GpioWrite(rst_, 1);
    DelayMs(10);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  uint8_t chip_id = 0;
  if (!GetChipId(chip_id)) {
    return false;
  }

  if (chip_id != kChipIdSgm41562BAndSa && chip_id != kChipIdSgm41562A &&
      chip_id != kChipIdSgm41562 && chip_id != kChipIdSgm41562S) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Unsupported SGM41562xx chip id: %#X\n", chip_id);
    return false;
  }

  if (!ResetRegisters()) {
    return false;
  }

  const ChipType detected_chip_type = DetectChipType(chip_id);
  if (detected_chip_type == ChipType::kUnknown) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Identify SGM41562xx failed (chip id: %#X)\n", chip_id);
    return false;
  }

  chip_type_ = detected_chip_type;
  const bool extended_register_map =
      chip_type_ == ChipType::kSgm41562S || chip_type_ == ChipType::kSgm41562Sa;
  const uint8_t* init_sequence =
      extended_register_map ? kInitSequenceS : kInitSequenceAb;
  const size_t init_sequence_size =
      extended_register_map ? sizeof(kInitSequenceS) : sizeof(kInitSequenceAb);
  if (!InitSequence(init_sequence, init_sequence_size)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "InitSequence failed\n");
    chip_type_ = ChipType::kUnknown;
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "Get %s chip id success (id: %#X)\n", ChipTypeToString(chip_type_),
      chip_id);
  return true;
}

bool Sgm41562xx::Deinit(bool delete_bus) {
  bool result = true;

  if (!ChipI2cGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
    result = false;
  }

  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }

  chip_type_ = ChipType::kUnknown;
  return result;
}

bool Sgm41562xx::GetChipId(uint8_t& chip_id) {
  uint8_t value = 0;
  if (!bus_->Read(static_cast<uint8_t>(Register::kChipId), &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read chip id failed\n");
    return false;
  }

  chip_id = value;
  return true;
}

Sgm41562xx::ChipType Sgm41562xx::GetChipType() const { return chip_type_; }

const char* Sgm41562xx::ChipTypeToString(ChipType chip_type) {
  switch (chip_type) {
    case ChipType::kSgm41562:
      return "SGM41562";
    case ChipType::kSgm41562A:
      return "SGM41562A";
    case ChipType::kSgm41562B:
      return "SGM41562B";
    case ChipType::kSgm41562S:
      return "SGM41562S";
    case ChipType::kSgm41562Sa:
      return "SGM41562SA";
    case ChipType::kUnknown:
    default:
      return "Unknown";
  }
}

Sgm41562xx::ChipType Sgm41562xx::DetectChipType(uint8_t chip_id) {
  switch (chip_id) {
    case kChipIdSgm41562A:
      return ChipType::kSgm41562A;
    case kChipIdSgm41562:
      return ChipType::kSgm41562;
    case kChipIdSgm41562S:
      return ChipType::kSgm41562S;
    case kChipIdSgm41562BAndSa:
      return DetectIdZeroChipType();
    default:
      return ChipType::kUnknown;
  }
}

Sgm41562xx::ChipType Sgm41562xx::DetectIdZeroChipType() {
  uint8_t charge_voltage_control = 0;
  uint8_t system_voltage_regulation = 0;
  if (!bus_->Read(static_cast<uint8_t>(Register::kChargeVoltageControl),
          &charge_voltage_control) ||
      !bus_->Read(static_cast<uint8_t>(Register::kSystemVoltageRegulation),
          &system_voltage_regulation)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "Read reset values failed\n");
    return ChipType::kUnknown;
  }

  // B和SA的芯片ID相同，软件复位后通过寄存器默认值区分型号
  if (charge_voltage_control == kSgm41562BChargeVoltageReset &&
      system_voltage_regulation == kSgm41562BSystemVoltageReset) {
    return ChipType::kSgm41562B;
  }
  if (charge_voltage_control == kSgm41562SaChargeVoltageReset &&
      system_voltage_regulation == kSgm41562SaSystemVoltageReset) {
    return ChipType::kSgm41562Sa;
  }

  LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
      "Unknown id 0x00 reset values (REG04: %#X, REG07: %#X)\n",
      charge_voltage_control, system_voltage_regulation);
  return ChipType::kUnknown;
}

bool Sgm41562xx::ResetRegisters() {
  uint8_t charge_current_control = 0;
  if (!bus_->Read(static_cast<uint8_t>(Register::kChargeCurrentControl),
          &charge_current_control) ||
      !bus_->Write(static_cast<uint8_t>(Register::kChargeCurrentControl),
          static_cast<uint8_t>(charge_current_control | kRegisterResetMask))) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "Reset registers failed\n");
    return false;
  }

  DelayMs(10);
  return true;
}

bool Sgm41562xx::UpdateRegisterBits(
    Register register_id, uint8_t mask, uint8_t value) {
  uint8_t current_value = 0;
  if (!bus_->Read(static_cast<uint8_t>(register_id), &current_value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read register failed\n");
    return false;
  }

  const uint8_t new_value =
      static_cast<uint8_t>((current_value & ~mask) | (value & mask));
  if (!bus_->Write(static_cast<uint8_t>(register_id), new_value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write register failed\n");
    return false;
  }

  return true;
}

bool Sgm41562xx::ReadRegister(
    Register register_id, uint8_t& value, const char* name) {
  if (!bus_->Read(static_cast<uint8_t>(register_id), &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Read %s failed (command: %#X)\n",
        name == nullptr ? "unknown register" : name,
        static_cast<unsigned int>(static_cast<uint8_t>(register_id)));
    return false;
  }

  return true;
}

bool Sgm41562xx::IsInitialized() {
  if (chip_type_ != ChipType::kUnknown) {
    return true;
  }

  LogMessage(LogLevel::kError, __FILE__, __LINE__, "Chip is not initialized\n");
  return false;
}

bool Sgm41562xx::GetFaultStatus(FaultStatus& status) {
  if (!IsInitialized()) {
    return false;
  }

  uint8_t fault_status = 0;
  if (!bus_->Read(static_cast<uint8_t>(Register::kFaultAndShippingControl),
          &fault_status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  ParseFaultStatus(fault_status, status);
  return true;
}

void Sgm41562xx::ParseFaultStatus(
    uint8_t fault_status, FaultStatus& status) {
  status.input_power_fault = (fault_status & 0x20) != 0;
  status.thermal_shutdown = (fault_status & 0x10) != 0;
  status.battery_overvoltage_fault = (fault_status & 0x08) != 0;
  status.safety_timer_expired = (fault_status & 0x04) != 0;
  status.ntc_hot = (fault_status & 0x02) != 0;
  status.ntc_cold = (fault_status & 0x01) != 0;
}

bool Sgm41562xx::SetChargeEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }

  return UpdateRegisterBits(Register::kPowerOnConfiguration, kChargeDisableMask,
      enable ? 0x00 : kChargeDisableMask);
}

bool Sgm41562xx::SetHighImpedanceModeEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kPowerOnConfiguration,
      kHighImpedanceEnableMask, enable ? kHighImpedanceEnableMask : 0x00);
}

bool Sgm41562xx::SetMinimumInputVoltageLimit(uint16_t voltage_mv) {
  if (!IsInitialized() ||
      !IsRegisterValueValid(voltage_mv, 3880, 5080, 80)) {
    return false;
  }
  const uint8_t value = static_cast<uint8_t>(((voltage_mv - 3880) / 80) << 4);
  return UpdateRegisterBits(
      Register::kInputSourceControl, kInputVoltageLimitMask, value);
}

bool Sgm41562xx::SetBatteryUndervoltageThreshold(uint16_t voltage_mv) {
  if (!IsInitialized() ||
      !IsRegisterValueValid(voltage_mv, 2400, 3030, 90)) {
    return false;
  }
  return UpdateRegisterBits(Register::kPowerOnConfiguration,
      kBatteryUndervoltageMask,
      static_cast<uint8_t>((voltage_mv - 2400) / 90));
}

bool Sgm41562xx::SetChargeVoltageLimit(uint16_t voltage_mv) {
  if (!IsInitialized()) {
    return false;
  }
  if (HasExtendedRegisterMap()) {
    if (!IsRegisterValueValid(voltage_mv, 3500, 4770, 10)) {
      return false;
    }
    return UpdateRegisterBits(Register::kChargeVoltageControl,
        kChargeVoltageSMask,
        static_cast<uint8_t>(((voltage_mv - 3500) / 10) << 1));
  }
  if (!IsRegisterValueValid(voltage_mv, 3600, 4545, 15)) {
    return false;
  }
  return UpdateRegisterBits(Register::kChargeVoltageControl,
      kChargeVoltageAbMask,
      static_cast<uint8_t>(((voltage_mv - 3600) / 15) << 2));
}

bool Sgm41562xx::SetSystemRegulationVoltage(uint16_t voltage_mv) {
  if (!IsInitialized()) {
    return false;
  }
  if (HasExtendedRegisterMap()) {
    if (!IsRegisterValueValid(voltage_mv, 3600, 5150, 50)) {
      return false;
    }
    return UpdateRegisterBits(Register::kSystemVoltageRegulation,
        kSystemVoltageSMask,
        static_cast<uint8_t>((voltage_mv - 3600) / 50));
  }
  if (!IsRegisterValueValid(voltage_mv, 4200, 4950, 50)) {
    return false;
  }
  return UpdateRegisterBits(Register::kSystemVoltageRegulation,
      kSystemVoltageAbMask,
      static_cast<uint8_t>((voltage_mv - 4200) / 50));
}

bool Sgm41562xx::SetInputCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized()) {
    return false;
  }
  if (HasExtendedRegisterMap()) {
    if (!IsRegisterValueValid(current_ma, 50, 980, 30)) {
      return false;
    }
    return UpdateRegisterBits(Register::kExtendedInputCurrentControl,
        kExtendedInputCurrentLimitMask,
        static_cast<uint8_t>(((current_ma - 50) / 30) << 3));
  }
  if (!IsRegisterValueValid(current_ma, 50, 500, 30)) {
    return false;
  }
  const bool limit_set = UpdateRegisterBits(Register::kInputSourceControl,
      kInputCurrentLimitMask,
      static_cast<uint8_t>((current_ma - 50) / 30));
  return limit_set &&
         UpdateRegisterBits(Register::kSystemStatus,
             kInputCurrentLimitReleaseMask | kInputCurrentLimitAdd200Mask,
             0x00);
}

bool Sgm41562xx::SetFastChargeCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized()) {
    return false;
  }

  uint8_t miscellaneous_configuration = 0;
  if (!ReadRegister(Register::kI2cAddressMiscellaneousConfiguration,
          miscellaneous_configuration, "REG0A miscellaneous configuration")) {
    return false;
  }

  const bool extended = HasExtendedRegisterMap();
  const bool fine_scale =
      (miscellaneous_configuration & kFineChargeCurrentScaleMask) != 0;
  const uint16_t minimum = fine_scale ? 2 : 8;
  const uint16_t maximum = fine_scale ? (extended ? 256 : 114)
                                      : (extended ? 1024 : 456);
  const uint16_t step = fine_scale ? 2 : 8;
  if (!IsRegisterValueValid(current_ma, minimum, maximum, step)) {
    return false;
  }
  const uint16_t unscaled_current_ma = fine_scale ? current_ma * 4 : current_ma;
  return UpdateRegisterBits(Register::kChargeCurrentControl,
      extended ? kFastChargeCurrentSMask : kFastChargeCurrentAbMask,
      static_cast<uint8_t>((unscaled_current_ma - 8) / 8));
}

bool Sgm41562xx::SetTerminationCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized() ||
      !IsRegisterValueValid(current_ma, 1, 31, 2)) {
    return false;
  }
  const bool current_set = UpdateRegisterBits(
      Register::kDischargeTerminationCurrent, kTerminationCurrentMask,
      static_cast<uint8_t>((current_ma - 1) / 2));
  if (!current_set || !HasExtendedRegisterMap()) {
    return current_set;
  }
  return UpdateRegisterBits(Register::kExtendedCurrentControl,
      kExtendedTerminationMultiplierMask, 0x00);
}

bool Sgm41562xx::SetPrechargeCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized() || !HasExtendedRegisterMap() ||
      !IsRegisterValueValid(current_ma, 1, 31, 2)) {
    return false;
  }
  const bool current_set = UpdateRegisterBits(Register::kExtendedCurrentControl,
      kExtendedPrechargeCurrentMask,
      static_cast<uint8_t>(((current_ma - 1) / 2) << 4));
  return current_set &&
         UpdateRegisterBits(Register::kExtendedCurrentControl,
             kExtendedPrechargeMultiplierMask, 0x00);
}

bool Sgm41562xx::SetDischargeCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized() ||
      !IsRegisterValueValid(current_ma, 400, 3200, 200)) {
    return false;
  }
  return UpdateRegisterBits(Register::kDischargeTerminationCurrent,
      kDischargeCurrentMask,
      static_cast<uint8_t>(((current_ma - 200) / 200) << 4));
}

bool Sgm41562xx::SetThermalRegulationThreshold(uint8_t temperature_c) {
  if (!IsInitialized() ||
      !IsRegisterValueValid(temperature_c, 60, 120, 20)) {
    return false;
  }
  const bool extended = HasExtendedRegisterMap();
  const uint8_t shift = extended ? 5 : 4;
  return UpdateRegisterBits(Register::kSystemVoltageRegulation,
      extended ? kThermalRegulationSMask : kThermalRegulationAbMask,
      static_cast<uint8_t>(((temperature_c - 60) / 20) << shift));
}

bool Sgm41562xx::SetWatchdogTimer(uint16_t timeout_s) {
  if (!IsInitialized()) {
    return false;
  }
  const uint16_t base_timeout_s = HasExtendedRegisterMap() ? 64 : 40;
  uint8_t setting = 0;
  if (timeout_s == base_timeout_s) {
    setting = 1;
  } else if (timeout_s == base_timeout_s * 2) {
    setting = 2;
  } else if (timeout_s == base_timeout_s * 4) {
    setting = 3;
  } else if (timeout_s != 0) {
    return false;
  }
  return UpdateRegisterBits(Register::kChargeTerminationTimerControl,
      kWatchdogMask, static_cast<uint8_t>(setting << 5));
}

bool Sgm41562xx::SetPrechargeToFastChargeThreshold(uint16_t voltage_mv) {
  if (!IsInitialized() || (voltage_mv != 2800 && voltage_mv != 3000)) {
    return false;
  }
  const Register register_id = HasExtendedRegisterMap()
                                   ? Register::kExtendedInputCurrentControl
                                   : Register::kChargeVoltageControl;
  const uint8_t mask = HasExtendedRegisterMap() ? kPrechargeThresholdSMask
                                                : kPrechargeThresholdAbMask;
  return UpdateRegisterBits(
      register_id, mask, voltage_mv == 3000 ? mask : 0x00);
}

bool Sgm41562xx::SetRechargeThreshold(uint16_t voltage_mv) {
  if (!IsInitialized() || (voltage_mv != 100 && voltage_mv != 200)) {
    return false;
  }
  return UpdateRegisterBits(Register::kChargeVoltageControl,
      kRechargeThresholdMask,
      voltage_mv == 200 ? kRechargeThresholdMask : 0x00);
}

bool Sgm41562xx::ResetWatchdogTimer() {
  if (!IsInitialized()) {
    return false;
  }
  const Register register_id = HasExtendedRegisterMap()
                                   ? Register::kExtendedInputCurrentControl
                                   : Register::kChargeCurrentControl;
  const uint8_t mask = HasExtendedRegisterMap() ? kWatchdogResetSMask
                                                : kWatchdogResetAbMask;
  return UpdateRegisterBits(register_id, mask, mask);
}

bool Sgm41562xx::SetWatchdogInDischargeEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kChargeTerminationTimerControl,
      kWatchdogInDischargeEnableMask,
      enable ? kWatchdogInDischargeEnableMask : 0x00);
}

bool Sgm41562xx::SetChargeTerminationEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kChargeTerminationTimerControl,
      kChargeTerminationEnableMask,
      enable ? kChargeTerminationEnableMask : 0x00);
}

bool Sgm41562xx::SetSafetyTimerEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kChargeTerminationTimerControl,
      kSafetyTimerEnableMask, enable ? kSafetyTimerEnableMask : 0x00);
}

bool Sgm41562xx::SetSafetyTimerDuration(uint8_t duration_hours) {
  if (!IsInitialized()) {
    return false;
  }
  uint8_t setting = 0;
  while (setting < sizeof(kSafetyTimerHours) &&
         kSafetyTimerHours[setting] != duration_hours) {
    ++setting;
  }
  if (setting == sizeof(kSafetyTimerHours)) {
    return false;
  }
  return UpdateRegisterBits(Register::kChargeTerminationTimerControl,
      kSafetyTimerSettingMask, static_cast<uint8_t>(setting << 1));
}

bool Sgm41562xx::SetChargeAfterTerminationEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kChargeTerminationTimerControl,
      kTerminationTimerEnableMask,
      enable ? kTerminationTimerEnableMask : 0x00);
}

bool Sgm41562xx::SetNtcEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kMiscellaneousOperationControl,
      kNtcEnableMask, enable ? kNtcEnableMask : 0x00);
}

bool Sgm41562xx::SetPpmSafetyTimerExtensionEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kMiscellaneousOperationControl,
      kSafetyTimerExtendedMask,
      enable ? kSafetyTimerExtendedMask : 0x00);
}

bool Sgm41562xx::SetInterruptEnable(
    InterruptType interrupt_type, bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  const uint8_t mask = static_cast<uint8_t>(interrupt_type);
  if (mask != static_cast<uint8_t>(InterruptType::kInputPowerGood) &&
      mask != static_cast<uint8_t>(InterruptType::kChargeComplete) &&
      mask != static_cast<uint8_t>(InterruptType::kChargeStatus) &&
      mask != static_cast<uint8_t>(InterruptType::kNtc) &&
      mask != static_cast<uint8_t>(InterruptType::kBatteryOvervoltage)) {
    return false;
  }
  return UpdateRegisterBits(Register::kMiscellaneousOperationControl, mask,
      enable ? 0x00 : mask);
}

bool Sgm41562xx::SetInputVoltageLoopEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  const uint8_t mask = HasExtendedRegisterMap()
                           ? kInputVoltageLoopDisableSMask
                           : kInputVoltageLoopDisableAbMask;
  return UpdateRegisterBits(Register::kSystemVoltageRegulation, mask,
      enable ? 0x00 : mask);
}

bool Sgm41562xx::SetPcbOvertemperatureProtectionEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  const Register register_id = HasExtendedRegisterMap()
                                   ? Register::kSystemStatus
                                   : Register::kSystemVoltageRegulation;
  const uint8_t mask = HasExtendedRegisterMap()
                           ? kPcbProtectionDisableSMask
                           : kPcbProtectionDisableAbMask;
  return UpdateRegisterBits(register_id, mask, enable ? 0x00 : mask);
}

bool Sgm41562xx::SetInputCurrentLimitReleaseEnable(bool enable) {
  if (!IsInitialized() || HasExtendedRegisterMap()) {
    return false;
  }
  return UpdateRegisterBits(Register::kSystemStatus,
      kInputCurrentLimitReleaseMask,
      enable ? kInputCurrentLimitReleaseMask : 0x00);
}

bool Sgm41562xx::SetInputCurrentLimitOffsetEnable(bool enable) {
  if (!IsInitialized() || HasExtendedRegisterMap()) {
    return false;
  }
  return UpdateRegisterBits(Register::kSystemStatus,
      kInputCurrentLimitAdd200Mask,
      enable ? kInputCurrentLimitAdd200Mask : 0x00);
}

bool Sgm41562xx::SetInputOvervoltageThreshold(uint16_t voltage_mv) {
  if (!IsInitialized() || !HasExtendedRegisterMap() ||
      (voltage_mv != 6000 && voltage_mv != 19000)) {
    return false;
  }
  return UpdateRegisterBits(Register::kSystemStatus,
      kInputOvervoltageSelectMask,
      voltage_mv == 19000 ? kInputOvervoltageSelectMask : 0x00);
}

bool Sgm41562xx::SetForcePowerPathSwitchEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kI2cAddressMiscellaneousConfiguration,
      kPowerPathSwitchForceMask,
      enable ? kPowerPathSwitchForceMask : 0x00);
}

bool Sgm41562xx::SetBatteryPowerEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kI2cAddressMiscellaneousConfiguration,
      kBatteryPowerDisableMask,
      enable ? 0x00 : kBatteryPowerDisableMask);
}

bool Sgm41562xx::SetInputOvervoltageProtectionEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kI2cAddressMiscellaneousConfiguration,
      kInputOvervoltageProtectionDisableMask,
      enable ? 0x00 : kInputOvervoltageProtectionDisableMask);
}

bool Sgm41562xx::SetQuarterChargeCurrentScaleEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kI2cAddressMiscellaneousConfiguration,
      kFineChargeCurrentScaleMask,
      enable ? kFineChargeCurrentScaleMask : 0x00);
}

bool Sgm41562xx::GetChipStatus(ChipStatus& status) {
  if (!IsInitialized()) {
    return false;
  }

  uint8_t chip_status = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Register::kSystemStatus), &chip_status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  ParseChipStatus(chip_status, status);
  return true;
}

void Sgm41562xx::ParseChipStatus(uint8_t chip_status, ChipStatus& status) {
  status.watchdog_expired = (chip_status & 0x80) != 0;
  status.charge_status = static_cast<ChargeStatus>((chip_status & 0x18) >> 3);
  status.power_path_management_active = (chip_status & 0x04) != 0;
  status.input_power_good = (chip_status & 0x02) != 0;
  status.thermal_regulation_active = (chip_status & 0x01) != 0;
}

bool Sgm41562xx::GetChargerConfig(ChargerConfig& config) {
  if (!IsInitialized()) {
    return false;
  }

  ChargerConfig new_config;
  if (!ReadInputConfig(new_config) || !ReadChargeConfig(new_config) ||
      !ReadProtectionConfig(new_config)) {
    return false;
  }

  config = new_config;
  return true;
}

bool Sgm41562xx::ReadInputConfig(ChargerConfig& config) {
  uint8_t input_source_control = 0;
  uint8_t power_on_configuration = 0;
  uint8_t system_status = 0;
  if (!ReadRegister(Register::kInputSourceControl, input_source_control,
          "REG00 input source control") ||
      !ReadRegister(Register::kPowerOnConfiguration, power_on_configuration,
          "REG01 power-on configuration") ||
      !ReadRegister(
          Register::kSystemStatus, system_status, "REG08 system status")) {
    return false;
  }

  config.charge_enabled = (power_on_configuration & kChargeDisableMask) == 0;
  config.high_impedance_enabled =
      (power_on_configuration & kHighImpedanceEnableMask) != 0;
  config.reset_pull_down_time_s =
      8 + 4 * ((power_on_configuration >> 6) & 0x03);
  config.battery_fet_off_time_s =
      (power_on_configuration & 0x20) != 0 ? 4 : 2;
  config.battery_undervoltage_threshold_mv =
      2400 + 90 * (power_on_configuration & kBatteryUndervoltageMask);
  config.minimum_input_voltage_limit_mv =
      3880 + 80 * ((input_source_control >> 4) & 0x0F);

  if (HasExtendedRegisterMap()) {
    uint8_t extended_input_current_control = 0;
    if (!ReadRegister(Register::kExtendedInputCurrentControl,
            extended_input_current_control,
            "REG0C extended input current control")) {
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
        (extended_input_current_control & 0x02) != 0 ? 3000 : 2800;
    config.input_overvoltage_threshold_mv =
        (system_status & kInputOvervoltageSelectMask) != 0 ? 19000 : 6000;
    config.pcb_overtemperature_protection_enabled =
        (system_status & kPcbProtectionDisableSMask) == 0;
  } else {
    config.input_current_limit_enabled =
        (system_status & kInputCurrentLimitReleaseMask) == 0;
    config.input_current_limit_200_ma_offset_enabled =
        (system_status & kInputCurrentLimitAdd200Mask) != 0;
    config.input_current_limit_ma = 50 + 30 * (input_source_control & 0x0F);
    if ((system_status & kInputCurrentLimitAdd200Mask) != 0) {
      config.input_current_limit_ma += 200;
    }
    config.input_overvoltage_threshold_mv =
        chip_type_ == ChipType::kSgm41562A ? 19000 : 6000;
  }
  return true;
}

bool Sgm41562xx::ReadChargeConfig(ChargerConfig& config) {
  uint8_t charge_current_control = 0;
  uint8_t discharge_termination_current = 0;
  uint8_t charge_voltage_control = 0;
  uint8_t miscellaneous_configuration = 0;
  if (!ReadRegister(Register::kChargeCurrentControl, charge_current_control,
          "REG02 charge current control") ||
      !ReadRegister(Register::kDischargeTerminationCurrent,
          discharge_termination_current,
          "REG03 discharge and termination current") ||
      !ReadRegister(Register::kChargeVoltageControl, charge_voltage_control,
          "REG04 charge voltage control") ||
      !ReadRegister(Register::kI2cAddressMiscellaneousConfiguration,
          miscellaneous_configuration, "REG0A miscellaneous configuration")) {
    return false;
  }

  const bool extended_register_map = HasExtendedRegisterMap();
  uint8_t fast_charge_current_code = charge_current_control & 0x3F;
  uint8_t extended_current_control = 0;
  if (extended_register_map) {
    fast_charge_current_code = charge_current_control & 0x7F;
    if (!ReadRegister(Register::kExtendedCurrentControl,
            extended_current_control, "REG0D extended current control")) {
      return false;
    }
  } else if (fast_charge_current_code > 56) {
    fast_charge_current_code = 56;
  }
  config.quarter_charge_current_scale_enabled =
      (miscellaneous_configuration & kFineChargeCurrentScaleMask) != 0;
  config.fast_charge_current_ma = 8 + 8 * fast_charge_current_code;
  if (config.quarter_charge_current_scale_enabled) {
    config.fast_charge_current_ma /= 4;
  }

  uint16_t termination_current_ma =
      1 + 2 * (discharge_termination_current & 0x0F);
  config.termination_current_multiplier_six_enabled =
      extended_register_map &&
      (extended_current_control & kExtendedTerminationMultiplierMask) != 0;
  if (config.termination_current_multiplier_six_enabled) {
    termination_current_ma *= 6;
  }
  config.termination_current_ma = termination_current_ma;
  config.discharge_current_limit_ma =
      200 + 200 * ((discharge_termination_current >> 4) & 0x0F);

  if (extended_register_map) {
    config.precharge_current_multiplier_six_enabled =
        (extended_current_control & kExtendedPrechargeMultiplierMask) != 0;
    config.precharge_current_ma =
        1 + 2 * ((extended_current_control >> 4) & 0x0F);
    if (config.precharge_current_multiplier_six_enabled) {
      config.precharge_current_ma *= 6;
    }
    config.charge_voltage_limit_mv =
        3500 + 10 * ((charge_voltage_control >> 1) & 0x7F);
  } else {
    config.charge_voltage_limit_mv =
        3600 + 15 * ((charge_voltage_control >> 2) & 0x3F);
    config.precharge_to_fast_charge_threshold_mv =
        (charge_voltage_control & kPrechargeThresholdAbMask) != 0 ? 3000
                                                                  : 2800;
  }
  config.recharge_threshold_mv =
      (charge_voltage_control & kRechargeThresholdMask) != 0 ? 200 : 100;
  return true;
}

bool Sgm41562xx::ReadProtectionConfig(ChargerConfig& config) {
  uint8_t charge_timer_control = 0;
  uint8_t miscellaneous_control = 0;
  uint8_t system_voltage_regulation = 0;
  uint8_t fault_and_shipping_control = 0;
  uint8_t miscellaneous_configuration = 0;
  if (!ReadRegister(Register::kChargeTerminationTimerControl,
          charge_timer_control, "REG05 charge termination and timer control") ||
      !ReadRegister(Register::kMiscellaneousOperationControl,
          miscellaneous_control, "REG06 miscellaneous operation control") ||
      !ReadRegister(Register::kSystemVoltageRegulation,
          system_voltage_regulation, "REG07 system voltage regulation") ||
      !ReadRegister(Register::kFaultAndShippingControl,
          fault_and_shipping_control, "REG09 fault and shipping control") ||
      !ReadRegister(Register::kI2cAddressMiscellaneousConfiguration,
          miscellaneous_configuration, "REG0A miscellaneous configuration")) {
    return false;
  }

  const bool extended_register_map = HasExtendedRegisterMap();
  if (extended_register_map) {
    config.system_voltage_regulation_mv =
        3600 + 50 * (system_voltage_regulation & 0x1F);
    config.input_voltage_loop_enabled =
        (system_voltage_regulation & kInputVoltageLoopDisableSMask) == 0;
    config.thermal_regulation_threshold_c =
        60 + 20 * ((system_voltage_regulation >> 5) & 0x03);
  } else {
    config.system_voltage_regulation_mv =
        4200 + 50 * (system_voltage_regulation & 0x0F);
    config.pcb_overtemperature_protection_enabled =
        (system_voltage_regulation & kPcbProtectionDisableAbMask) == 0;
    config.input_voltage_loop_enabled =
        (system_voltage_regulation & kInputVoltageLoopDisableAbMask) == 0;
    config.thermal_regulation_threshold_c =
        60 + 20 * ((system_voltage_regulation >> 4) & 0x03);
  }

  const uint8_t watchdog_setting = (charge_timer_control & kWatchdogMask) >> 5;
  config.watchdog_in_discharge_enabled =
      (charge_timer_control & kWatchdogInDischargeEnableMask) != 0;
  config.watchdog_enabled = watchdog_setting != 0;
  if (config.watchdog_enabled) {
    const uint16_t watchdog_base_s = extended_register_map ? 64 : 40;
    config.watchdog_timeout_s = watchdog_base_s << (watchdog_setting - 1);
  }
  config.charge_termination_enabled =
      (charge_timer_control & kChargeTerminationEnableMask) != 0;
  config.safety_timer_enabled =
      (charge_timer_control & kSafetyTimerEnableMask) != 0;
  const uint8_t safety_timer_setting =
      (charge_timer_control & kSafetyTimerSettingMask) >> 1;
  config.safety_timer_hours = kSafetyTimerHours[safety_timer_setting];
  config.charge_after_termination_enabled =
      (charge_timer_control & kTerminationTimerEnableMask) != 0;
  config.safety_timer_extended_in_ppm =
      (miscellaneous_control & kSafetyTimerExtendedMask) != 0;
  config.ntc_enabled = (miscellaneous_control & kNtcEnableMask) != 0;
  config.shipping_mode_enabled =
      (miscellaneous_control & kShippingModeEnableMask) != 0;
  config.input_power_good_interrupt_enabled =
      (miscellaneous_control & 0x10) == 0;
  config.charge_complete_interrupt_enabled =
      (miscellaneous_control & 0x08) == 0;
  config.charge_status_interrupt_enabled =
      (miscellaneous_control & 0x04) == 0;
  config.ntc_interrupt_enabled = (miscellaneous_control & 0x02) == 0;
  config.battery_overvoltage_interrupt_enabled =
      (miscellaneous_control & 0x01) == 0;
  config.shipping_mode_delay_s = static_cast<uint8_t>(
      1U << ((fault_and_shipping_control & kShippingModeDelayMask) >> 6));
  config.i2c_address = (miscellaneous_configuration >> 5) & 0x07;
  config.force_power_path_switch_enabled =
      (miscellaneous_configuration & kPowerPathSwitchForceMask) != 0;
  config.battery_power_enabled =
      (miscellaneous_configuration & kBatteryPowerDisableMask) == 0;
  config.input_overvoltage_protection_enabled =
      (miscellaneous_configuration &
          kInputOvervoltageProtectionDisableMask) == 0;
  return true;
}

bool Sgm41562xx::HasExtendedRegisterMap() const {
  return chip_type_ == ChipType::kSgm41562S ||
         chip_type_ == ChipType::kSgm41562Sa;
}

bool Sgm41562xx::SetShippingModeEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }

  return UpdateRegisterBits(Register::kMiscellaneousOperationControl,
      kShippingModeEnableMask, enable ? kShippingModeEnableMask : 0x00);
}

bool Sgm41562xx::SetShippingModeDelay(ShippingModeDelay delay) {
  if (!IsInitialized()) {
    return false;
  }

  const uint8_t delay_value = static_cast<uint8_t>(delay);
  if (delay_value > static_cast<uint8_t>(ShippingModeDelay::k8Seconds)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Shipping mode delay out of range\n");
    return false;
  }

  return UpdateRegisterBits(Register::kFaultAndShippingControl,
      kShippingModeDelayMask, static_cast<uint8_t>(delay_value << 6));
}

}  // namespace cpp_bus_driver
