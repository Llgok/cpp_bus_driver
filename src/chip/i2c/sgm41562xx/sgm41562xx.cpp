/*
 * @Description: SGM41562 系列电池充电管理芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-09-05 14:56:53
 * @License: GPL 3.0
 */
#include "chip/i2c/sgm41562xx/sgm41562xx.h"

namespace cpp_bus_driver {
namespace {
constexpr uint8_t kRegisterResetMask = 0x80;
constexpr uint8_t kChargeDisableMask = 0x08;
constexpr uint8_t kHighImpedanceEnableMask = 0x10;
constexpr uint8_t kBatteryUndervoltageMask = 0x07;
constexpr uint8_t kInputVoltageLimitMask = 0xF0;
constexpr uint8_t kTerminationCurrentMask = 0x0F;
constexpr uint8_t kDischargeCurrentMask = 0xF0;
constexpr uint8_t kRechargeThresholdMask = 0x01;
constexpr uint8_t kWatchdogInDischargeEnableMask = 0x80;
constexpr uint8_t kChargeTerminationEnableMask = 0x10;
constexpr uint8_t kSafetyTimerEnableMask = 0x08;
constexpr uint8_t kSafetyTimerSettingMask = 0x06;
constexpr uint8_t kTerminationTimerEnableMask = 0x01;
constexpr uint8_t kNtcEnableMask = 0x80;
constexpr uint8_t kSafetyTimerExtendedMask = 0x40;
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
  return value >= minimum && value <= maximum && (value - minimum) % step == 0;
}
}  // namespace

const Sgm41562xx::ModelDriver* Sgm41562xx::GetModelDriver(ChipModel chip_model) {
  static const Sgm41562Driver kSgm41562;
  static const Sgm41562sDriver kSgm41562s;
  switch (chip_model) {
    case ChipModel::kSgm41562:
    case ChipModel::kSgm41562A:
    case ChipModel::kSgm41562B:
      return &kSgm41562;
    case ChipModel::kSgm41562S:
    case ChipModel::kSgm41562Sa:
      return &kSgm41562s;
    case ChipModel::kUnknown:
    default:
      return nullptr;
  }
}

uint32_t Sgm41562xx::MakeFeatureMask(std::initializer_list<Feature> features) {
  uint32_t mask = 0;
  for (Feature feature : features) {
    mask |= uint32_t{1} << static_cast<uint8_t>(feature);
  }
  return mask;
}

Sgm41562xx::ModelDriver::ModelDriver(std::initializer_list<Feature> features)
    : feature_mask_(MakeFeatureMask(features)) {}

bool Sgm41562xx::Init(int32_t freq_hz) {
  chip_model_ = ChipModel::kUnknown;
  model_driver_ = nullptr;

  if (!I2cChipBase::Init(freq_hz)) {
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

  const ChipModel detected_chip_model = DetectChipModel(chip_id);
  if (detected_chip_model == ChipModel::kUnknown) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Identify SGM41562xx failed (chip id: %#X)\n", chip_id);
    return false;
  }

  chip_model_ = detected_chip_model;
  model_driver_ = GetModelDriver(chip_model_);
  if (model_driver_ == nullptr || !model_driver_->Init(*this)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "InitSequence failed\n");
    chip_model_ = ChipModel::kUnknown;
    model_driver_ = nullptr;
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "Get %s chip id success (id: %#X)\n", ChipModelToString(chip_model_),
      chip_id);
  return true;
}

bool Sgm41562xx::Deinit(bool delete_bus) {
  bool result = true;

  if (!I2cChipBase::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
    result = false;
  }

  chip_model_ = ChipModel::kUnknown;
  model_driver_ = nullptr;
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

Sgm41562xx::ChipModel Sgm41562xx::GetChipModel() const { return chip_model_; }

bool Sgm41562xx::HasFeature(Feature feature) const {
  const auto bit = static_cast<uint8_t>(feature);
  if (chip_model_ == ChipModel::kUnknown || model_driver_ == nullptr ||
      bit >= 32) {
    return false;
  }
  return (model_driver_->feature_mask_ & (uint32_t{1} << bit)) != 0;
}

const char* Sgm41562xx::ChipModelToString(ChipModel chip_model) {
  switch (chip_model) {
    case ChipModel::kSgm41562:
      return "SGM41562";
    case ChipModel::kSgm41562A:
      return "SGM41562A";
    case ChipModel::kSgm41562B:
      return "SGM41562B";
    case ChipModel::kSgm41562S:
      return "SGM41562S";
    case ChipModel::kSgm41562Sa:
      return "SGM41562SA";
    case ChipModel::kUnknown:
    default:
      return "Unknown";
  }
}

Sgm41562xx::ChipModel Sgm41562xx::DetectChipModel(uint8_t chip_id) {
  switch (chip_id) {
    case kChipIdSgm41562A:
      return ChipModel::kSgm41562A;
    case kChipIdSgm41562:
      return ChipModel::kSgm41562;
    case kChipIdSgm41562S:
      return ChipModel::kSgm41562S;
    case kChipIdSgm41562BAndSa:
      return DetectIdZeroChipModel();
    default:
      return ChipModel::kUnknown;
  }
}

Sgm41562xx::ChipModel Sgm41562xx::DetectIdZeroChipModel() {
  uint8_t charge_voltage_control = 0;
  uint8_t system_voltage_regulation = 0;
  if (!bus_->Read(static_cast<uint8_t>(Register::kChargeVoltageControl),
          &charge_voltage_control) ||
      !bus_->Read(static_cast<uint8_t>(Register::kSystemVoltageRegulation),
          &system_voltage_regulation)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "Read reset values failed\n");
    return ChipModel::kUnknown;
  }

  // B和SA的芯片ID相同，软件复位后通过寄存器默认值区分型号
  if (charge_voltage_control == kSgm41562BChargeVoltageReset &&
      system_voltage_regulation == kSgm41562BSystemVoltageReset) {
    return ChipModel::kSgm41562B;
  }
  if (charge_voltage_control == kSgm41562SaChargeVoltageReset &&
      system_voltage_regulation == kSgm41562SaSystemVoltageReset) {
    return ChipModel::kSgm41562Sa;
  }

  LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
      "Unknown id 0x00 reset values (REG04: %#X, REG07: %#X)\n",
      charge_voltage_control, system_voltage_regulation);
  return ChipModel::kUnknown;
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

bool Sgm41562xx::SetRegisterField(const RegisterField& field, uint16_t value) {
  if (!IsRegisterValueValid(value, field.minimum, field.maximum, field.step)) {
    return false;
  }
  return UpdateRegisterBits(field.register_id, field.mask,
      static_cast<uint8_t>(
          ((value - field.minimum) / field.step) << field.shift));
}

uint16_t Sgm41562xx::DecodeRegisterField(
    const RegisterField& field, uint8_t register_value) {
  const uint16_t value =
      field.minimum +
      field.step * ((register_value & field.mask) >> field.shift);
  return value > field.maximum ? field.maximum : value;
}

bool Sgm41562xx::ModelDriver::FinishTerminationCurrentLimit(Sgm41562xx&) const {
  return true;
}

bool Sgm41562xx::ModelDriver::SetPrechargeCurrentLimit(
    Sgm41562xx&, uint16_t) const {
  return false;
}

bool Sgm41562xx::ModelDriver::SetInputCurrentLimitReleaseEnable(
    Sgm41562xx&, bool) const {
  return false;
}

bool Sgm41562xx::ModelDriver::SetInputCurrentLimitOffsetEnable(
    Sgm41562xx&, bool) const {
  return false;
}

bool Sgm41562xx::ModelDriver::SetInputOvervoltageThreshold(
    Sgm41562xx&, uint16_t) const {
  return false;
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
  if (chip_model_ != ChipModel::kUnknown && model_driver_ != nullptr) {
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

void Sgm41562xx::ParseFaultStatus(uint8_t fault_status, FaultStatus& status) {
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
  if (!IsInitialized() || !IsRegisterValueValid(voltage_mv, 3880, 5080, 80)) {
    return false;
  }
  const uint8_t value = static_cast<uint8_t>(((voltage_mv - 3880) / 80) << 4);
  return UpdateRegisterBits(
      Register::kInputSourceControl, kInputVoltageLimitMask, value);
}

bool Sgm41562xx::SetBatteryUndervoltageThreshold(uint16_t voltage_mv) {
  if (!IsInitialized() || !IsRegisterValueValid(voltage_mv, 2400, 3030, 90)) {
    return false;
  }
  return UpdateRegisterBits(Register::kPowerOnConfiguration,
      kBatteryUndervoltageMask, static_cast<uint8_t>((voltage_mv - 2400) / 90));
}

bool Sgm41562xx::SetChargeVoltageLimit(uint16_t voltage_mv) {
  if (!IsInitialized()) {
    return false;
  }
  return model_driver_->SetChargeVoltageLimit(*this, voltage_mv);
}

bool Sgm41562xx::SetSystemRegulationVoltage(uint16_t voltage_mv) {
  if (!IsInitialized()) {
    return false;
  }
  return model_driver_->SetSystemRegulationVoltage(*this, voltage_mv);
}

bool Sgm41562xx::SetInputCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized()) {
    return false;
  }
  return model_driver_->SetInputCurrentLimit(*this, current_ma);
}

bool Sgm41562xx::SetFastChargeCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized()) {
    return false;
  }
  return model_driver_->SetFastChargeCurrentLimit(*this, current_ma);
}

bool Sgm41562xx::SetTerminationCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized() || !IsRegisterValueValid(current_ma, 1, 31, 2)) {
    return false;
  }
  const bool current_set =
      UpdateRegisterBits(Register::kDischargeTerminationCurrent,
          kTerminationCurrentMask, static_cast<uint8_t>((current_ma - 1) / 2));
  return current_set && model_driver_->FinishTerminationCurrentLimit(*this);
}

bool Sgm41562xx::SetPrechargeCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized() || !HasFeature(Feature::kPrechargeCurrent)) {
    return false;
  }
  return model_driver_->SetPrechargeCurrentLimit(*this, current_ma);
}

bool Sgm41562xx::SetDischargeCurrentLimit(uint16_t current_ma) {
  if (!IsInitialized() || !IsRegisterValueValid(current_ma, 400, 3200, 200)) {
    return false;
  }
  return UpdateRegisterBits(Register::kDischargeTerminationCurrent,
      kDischargeCurrentMask,
      static_cast<uint8_t>(((current_ma - 200) / 200) << 4));
}

bool Sgm41562xx::SetThermalRegulationThreshold(uint8_t temperature_c) {
  if (!IsInitialized()) {
    return false;
  }
  return model_driver_->SetThermalRegulationThreshold(*this, temperature_c);
}

bool Sgm41562xx::SetWatchdogTimer(uint16_t timeout_s) {
  if (!IsInitialized()) {
    return false;
  }
  return model_driver_->SetWatchdogTimer(*this, timeout_s);
}

bool Sgm41562xx::SetPrechargeToFastChargeThreshold(uint16_t voltage_mv) {
  if (!IsInitialized() || (voltage_mv != 2800 && voltage_mv != 3000)) {
    return false;
  }
  return model_driver_->SetPrechargeToFastChargeThreshold(*this, voltage_mv);
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
  return model_driver_->ResetWatchdogTimer(*this);
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
      kTerminationTimerEnableMask, enable ? kTerminationTimerEnableMask : 0x00);
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
      kSafetyTimerExtendedMask, enable ? kSafetyTimerExtendedMask : 0x00);
}

bool Sgm41562xx::SetInterruptEnable(InterruptType interrupt_type, bool enable) {
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
  return UpdateRegisterBits(
      Register::kMiscellaneousOperationControl, mask, enable ? 0x00 : mask);
}

bool Sgm41562xx::SetInputVoltageLoopEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return model_driver_->SetInputVoltageLoopEnable(*this, enable);
}

bool Sgm41562xx::SetPcbOvertemperatureProtectionEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return model_driver_->SetPcbOvertemperatureProtectionEnable(*this, enable);
}

bool Sgm41562xx::SetInputCurrentLimitReleaseEnable(bool enable) {
  if (!IsInitialized() || !HasFeature(Feature::kInputCurrentLimitRelease)) {
    return false;
  }
  return model_driver_->SetInputCurrentLimitReleaseEnable(*this, enable);
}

bool Sgm41562xx::SetInputCurrentLimitOffsetEnable(bool enable) {
  if (!IsInitialized() || !HasFeature(Feature::kInputCurrentLimitOffset)) {
    return false;
  }
  return model_driver_->SetInputCurrentLimitOffsetEnable(*this, enable);
}

bool Sgm41562xx::SetInputOvervoltageThreshold(uint16_t voltage_mv) {
  if (!IsInitialized() || !HasFeature(Feature::kInputOvervoltageThreshold)) {
    return false;
  }
  return model_driver_->SetInputOvervoltageThreshold(*this, voltage_mv);
}

bool Sgm41562xx::SetForcePowerPathSwitchEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kI2cAddressMiscellaneousConfiguration,
      kPowerPathSwitchForceMask, enable ? kPowerPathSwitchForceMask : 0x00);
}

bool Sgm41562xx::SetBatteryPowerEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }
  return UpdateRegisterBits(Register::kI2cAddressMiscellaneousConfiguration,
      kBatteryPowerDisableMask, enable ? 0x00 : kBatteryPowerDisableMask);
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
      kFineChargeCurrentScaleMask, enable ? kFineChargeCurrentScaleMask : 0x00);
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
  config.battery_fet_off_time_s = (power_on_configuration & 0x20) != 0 ? 4 : 2;
  config.battery_undervoltage_threshold_mv =
      2400 + 90 * (power_on_configuration & kBatteryUndervoltageMask);
  config.minimum_input_voltage_limit_mv =
      3880 + 80 * ((input_source_control >> 4) & 0x0F);

  return model_driver_->ReadInputConfig(
      *this, input_source_control, system_status, config);
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

  config.quarter_charge_current_scale_enabled =
      (miscellaneous_configuration & kFineChargeCurrentScaleMask) != 0;
  config.termination_current_ma =
      1 + 2 * (discharge_termination_current & 0x0F);
  config.discharge_current_limit_ma =
      200 + 200 * ((discharge_termination_current >> 4) & 0x0F);
  config.recharge_threshold_mv =
      (charge_voltage_control & kRechargeThresholdMask) != 0 ? 200 : 100;
  return model_driver_->ReadChargeConfig(
      *this, charge_current_control, charge_voltage_control, config);
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

  model_driver_->ParseProtectionConfig(
      charge_timer_control, system_voltage_regulation, config);

  config.watchdog_in_discharge_enabled =
      (charge_timer_control & kWatchdogInDischargeEnableMask) != 0;
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
  config.charge_status_interrupt_enabled = (miscellaneous_control & 0x04) == 0;
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
      (miscellaneous_configuration & kInputOvervoltageProtectionDisableMask) ==
      0;
  return true;
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
