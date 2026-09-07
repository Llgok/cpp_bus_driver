/*
 * @Description: BQ2589x 系列充电管理芯片驱动
 * @License: GPL 3.0
 */
#include "chip/i2c/bq2589x/bq2589x.h"

namespace cpp_bus_driver {
namespace {

constexpr uint16_t kBoostMinimumBatteryVoltagesMv[] = {2900, 2500};
constexpr uint16_t kPrechargeThresholdsMv[] = {2800, 3000};
constexpr uint16_t kRechargeOffsetsMv[] = {100, 200};
constexpr uint16_t kWatchdogTimeoutsS[] = {0, 40, 80, 160};
constexpr uint16_t kFastChargeTimersHours[] = {5, 8, 12, 20};
constexpr uint16_t kThermalThresholdsC[] = {60, 80, 100, 120};

}  // namespace

uint32_t Bq2589x::MakeFeatureMask(std::initializer_list<Feature> features) {
  uint32_t mask = 0;
  for (Feature feature : features) {
    mask |= uint32_t{1} << static_cast<uint8_t>(feature);
  }
  return mask;
}

const Bq2589x::ModelDriver* const* Bq2589x::GetModelDrivers(size_t& count) {
  static const Bq25890Driver kBq25890;
  static const Bq25890hDriver kBq25890h;
  static const Bq25892Driver kBq25892;
  static const Bq25895Driver kBq25895;
  static const Bq25895mDriver kBq25895m;
  static const Bq25896Driver kBq25896;
  static const Bq25898Driver kBq25898;
  static const Bq25898cDriver kBq25898c;
  static const Bq25898dDriver kBq25898d;
  static const ModelDriver* const drivers[] = {&kBq25890, &kBq25890h, &kBq25892,
      &kBq25895, &kBq25895m, &kBq25896, &kBq25898, &kBq25898c, &kBq25898d};
  count = sizeof(drivers) / sizeof(drivers[0]);
  return drivers;
}

const Bq2589x::ModelDriver* Bq2589x::GetModelDriver(ChipModel model) {
  size_t count = 0;
  const ModelDriver* const* drivers = GetModelDrivers(count);
  for (size_t i = 0; i < count; ++i) {
    if (drivers[i]->config_.model == model) {
      return drivers[i];
    }
  }
  return nullptr;
}

Bq2589x::ModelDriver::ModelDriver(ChipModel model, uint8_t address,
    uint8_t part_number, uint8_t device_revision,
    std::initializer_list<Feature> features, bool jeita_profile_valid)
    : config_{model, address, part_number, device_revision,
          MakeFeatureMask(features), jeita_profile_valid} {}

bool Bq2589x::ModelDriver::SetBoostCurrentLimit(Bq2589x&, uint16_t) const {
  return false;
}

bool Bq2589x::ModelDriver::GetBoostCurrentLimit(Bq2589x&, uint16_t&) const {
  return false;
}

bool Bq2589x::ModelDriver::SetDpDmDac(Bq2589x&, bool, DpDmVoltage) const {
  return false;
}

bool Bq2589x::ModelDriver::GetDpDmDac(Bq2589x&, bool, DpDmVoltage&) const {
  return false;
}

Bq2589x::ChipStatus Bq2589x::ModelDriver::DecodeChipStatus(
    uint8_t value) const {
  ChipStatus status;
  status.vbus_status = static_cast<VbusStatus>((value >> 5) & 0x07);
  status.charge_status = static_cast<ChargeStatus>((value >> 3) & 0x03);
  status.power_good = (value & 0x04) != 0;
  status.system_minimum_voltage_regulation = (value & 0x01) != 0;
  status.raw = value;
  return status;
}

Bq2589x::DpmStatus Bq2589x::ModelDriver::DecodeDpmStatus(uint8_t value) const {
  DpmStatus status;
  status.vindpm_active = (value & 0x80) != 0;
  status.iindpm_active = (value & 0x40) != 0;
  status.input_current_limit_code = value & 0x3F;
  status.input_current_limit_valid = true;
  status.input_current_limit_ma = 100 + status.input_current_limit_code * 50;
  return status;
}

bool Bq2589x::ModelDriver::ReadAdcRegisters(
    I2cBusBase& bus, uint8_t (&data)[5]) const {
  return bus.Read(static_cast<uint8_t>(Register::kReg0e), data, sizeof(data));
}

Bq2589x::NtcFault Bq2589x::ModelDriver::DecodeNtcFault(uint8_t code) const {
  switch (code) {
    case 0:
    case 2:
    case 3:
    case 5:
    case 6:
      return static_cast<NtcFault>(code);
    default:
      return NtcFault::kUnknown;
  }
}

bool Bq2589x::Init(int32_t freq_hz) {
  if (bus_ == nullptr || freq_hz <= 0 || freq_hz > 400000 ||
      (device_address_ != 0x6A && device_address_ != 0x6B)) {
    return false;
  }
  if (initialized_) {
    return true;
  }
  if (bus_cleanup_required_ && !I2cChipBase::Deinit(false)) {
    return false;
  }
  chip_info_ = {};
  model_driver_ = nullptr;
  bus_cleanup_required_ = true;
  if (!I2cChipBase::Init(freq_hz)) {
    I2cChipBase::Deinit(false);
    bus_initialized_ = false;
    return false;
  }
  bus_initialized_ = true;
  uint8_t device_id = 0;
  if (!ReadRegister(Register::kReg14, device_id)) {
    I2cChipBase::Deinit(false);
    bus_initialized_ = false;
    return false;
  }
  const ChipInfo info = DecodeChipInfo(device_id);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "BQ2589x device ID (address: %#X, REG14: %#04X, PN: %#X, revision: "
      "%#X)\n",
      device_address_, device_id, info.part_number, info.device_revision);
  chip_info_ = info;
  if (info.model == ChipModel::kUnknown ||
      (requested_model_ != ChipModel::kUnknown &&
          requested_model_ != info.model)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "BQ2589x unknown, ambiguous or mismatched model (PN: %#X, revision: "
        "%#X)\n",
        info.part_number, info.device_revision);
    // 只读取配置和器件信息用于诊断，不读取会清除故障记录的 REG0C。
    for (uint8_t sample = 0; sample < 3; ++sample) {
      for (const Register reg : {Register::kReg00, Register::kReg03,
               Register::kReg07, Register::kReg14}) {
        uint8_t value = 0;
        if (bus_->Read(static_cast<uint8_t>(reg), &value)) {
          LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
              "BQ2589x diagnostic sample %u: REG%02X = %#04X\n",
              static_cast<unsigned>(sample + 1), static_cast<unsigned>(reg),
              value);
        } else {
          LogMessage(LogLevel::kError, __FILE__, __LINE__,
              "BQ2589x diagnostic sample %u: REG%02X read failed\n",
              static_cast<unsigned>(sample + 1), static_cast<unsigned>(reg));
        }
      }
      if (sample < 2) {
        DelayMs(2);
      }
    }
    I2cChipBase::Deinit(false);
    bus_initialized_ = false;
    return false;
  }
  model_driver_ = GetModelDriver(info.model);
  if (model_driver_ == nullptr || !model_driver_->Init(*this)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "BQ2589x InitSequence failed\n");
    model_driver_ = nullptr;
    I2cChipBase::Deinit(false);
    bus_initialized_ = false;
    return false;
  }
  initialized_ = true;
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "BQ2589x identified (model: %u)\n", static_cast<unsigned>(info.model));
  return true;
}

bool Bq2589x::Deinit(bool delete_bus) {
  initialized_ = false;
  model_driver_ = nullptr;
  bus_initialized_ = false;
  if (bus_cleanup_required_ && !I2cChipBase::Deinit(delete_bus)) {
    return false;
  }
  if (delete_bus) {
    bus_cleanup_required_ = false;
  }
  chip_info_ = {};
  return true;
}

bool Bq2589x::IsSupported() const {
  return initialized_ && model_driver_ != nullptr;
}

uint8_t Bq2589x::GetDefaultAddress(ChipModel model) {
  const ModelDriver* driver = GetModelDriver(model);
  return driver != nullptr ? driver->config_.address : 0x6B;
}

bool Bq2589x::HasFeature(Feature feature) const {
  const auto bit = static_cast<uint8_t>(feature);
  if (!IsSupported() || bit >= 32) {
    return false;
  }
  return (model_driver_->config_.feature_mask & (uint32_t{1} << bit)) != 0;
}

Bq2589x::ChipInfo Bq2589x::DecodeChipInfo(uint8_t value) const {
  ChipInfo info;
  info.part_number = (value >> 3) & 0x07;
  info.device_revision = value & 0x03;
  info.jeita_profile = (value & 0x04) != 0;
  const ModelDriver* detected_driver = nullptr;
  const ModelDriver* requested_driver = nullptr;
  size_t count = 0;
  size_t matches = 0;
  const ModelDriver* const* drivers = GetModelDrivers(count);
  // 仅接受各型号手册列出的 PN/REV；TS_PROFILE 的实读值不参与身份识别。
  for (size_t i = 0; i < count; ++i) {
    const ModelConfig& config = drivers[i]->config_;
    if (config.part_number == info.part_number &&
        config.device_revision == info.device_revision) {
      detected_driver = drivers[i];
      ++matches;
      if (config.model == requested_model_) {
        requested_driver = drivers[i];
      }
    }
  }
  info.model_is_ambiguous = matches > 1;
  if (info.model_is_ambiguous) {
    // BQ25892 与 BQ25898 的身份相同；显式选择后仍报告重合标志。
    detected_driver = requested_driver;
  }
  if (detected_driver != nullptr &&
      device_address_ == detected_driver->config_.address) {
    info.model = detected_driver->config_.model;
  }
  info.jeita_profile_valid = info.model_is_ambiguous ||
                             (info.model != ChipModel::kUnknown &&
                                 detected_driver->config_.jeita_profile_valid);
  if (!info.jeita_profile_valid) {
    info.jeita_profile = false;
  }
  return info;
}

bool Bq2589x::GetChipInfo(ChipInfo& info) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kReg14, value)) {
    return false;
  }
  info = DecodeChipInfo(value);
  return true;
}

bool Bq2589x::GetChipId(uint8_t& part_number) {
  ChipInfo info;
  if (!GetChipInfo(info)) {
    return false;
  }
  part_number = info.part_number;
  return true;
}

bool Bq2589x::ReadRegister(Register reg, uint8_t& value) {
  if (!bus_initialized_ || bus_ == nullptr ||
      (reg != Register::kReg14 && !IsSupported())) {
    return false;
  }
  if (reg == Register::kReg10 && !HasFeature(Feature::kTsAdc)) {
    return false;
  }
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(reg), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "BQ2589x register read failed (register: %#X)\n",
        static_cast<unsigned>(reg));
    return false;
  }
  value = buffer;
  return true;
}

bool Bq2589x::ReadField(
    Register reg, uint8_t mask, uint8_t shift, uint8_t& value) {
  uint8_t buffer = 0;
  if (!ReadRegister(reg, buffer)) {
    return false;
  }
  value = (buffer & mask) >> shift;
  return true;
}

bool Bq2589x::ReadFlag(Register reg, uint8_t mask, bool& value, bool inverted) {
  uint8_t buffer = 0;
  if (!ReadRegister(reg, buffer)) {
    return false;
  }
  value = ((buffer & mask) != 0) != inverted;
  return true;
}

bool Bq2589x::UpdateRegisterBits(Register reg, uint8_t mask, uint8_t value) {
  if (!IsSupported() || mask == 0 || (value & ~mask) != 0) {
    return false;
  }
  uint8_t command_mask = 0;
  switch (reg) {
    case Register::kReg02:
      command_mask = 0x82;  // CONV_START、FORCE_DPDM 自清零命令。
      break;
    case Register::kReg03:
      command_mask = 0x40;  // WD_RST 自清零命令。
      break;
    case Register::kReg09:
      // 只清除当前型号具备的 FORCE_ICO、PUMPX_UP 和 PUMPX_DN 命令位。
      command_mask = HasFeature(Feature::kInputCurrentOptimizer) ? 0x80 : 0x00;
      if (HasFeature(Feature::kCurrentPulseControl)) {
        command_mask |= 0x03;
      }
      break;
    case Register::kReg14:
      command_mask = 0x80;  // 仅 REG_RST 可写，其余字段只读。
      break;
    default:
      break;
  }
  uint8_t buffer = 0;
  if (reg != Register::kReg14 && !ReadRegister(reg, buffer)) {
    return false;
  }
  // 保留无关配置和保留位；自清零命令即使读到 1，也不能随普通配置写回再次触发。
  buffer = static_cast<uint8_t>((buffer & ~(mask | command_mask)) | value);
  if (!bus_->Write(static_cast<uint8_t>(reg), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "BQ2589x register write failed (register: %#X)\n",
        static_cast<unsigned>(reg));
    return false;
  }
  return true;
}

bool Bq2589x::SetLinearField(Register reg, uint8_t mask, uint8_t shift,
    uint16_t value, uint16_t minimum, uint16_t maximum, uint16_t step) {
  if (value < minimum || value > maximum) {
    return false;
  }
  return UpdateRegisterBits(
      reg, mask, static_cast<uint8_t>(((value - minimum) / step) << shift));
}

bool Bq2589x::GetLinearField(Register reg, uint8_t mask, uint8_t shift,
    uint16_t minimum, uint16_t maximum, uint16_t step, uint16_t& value) {
  uint8_t code = 0;
  if (!ReadField(reg, mask, shift, code)) {
    return false;
  }
  const uint16_t result = minimum + code * step;
  if (result > maximum) {
    return false;
  }
  value = result;
  return true;
}

bool Bq2589x::SetTableField(Register reg, uint8_t mask, uint8_t shift,
    uint16_t value, const uint16_t* table, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (table[i] == value) {
      return UpdateRegisterBits(reg, mask, static_cast<uint8_t>(i << shift));
    }
  }
  return false;
}

bool Bq2589x::GetTableField(Register reg, uint8_t mask, uint8_t shift,
    const uint16_t* table, size_t count, uint16_t& value) {
  uint8_t code = 0;
  if (!ReadField(reg, mask, shift, code) || code >= count) {
    return false;
  }
  value = table[code];
  return true;
}

bool Bq2589x::WaitForClear(Register reg, uint8_t mask, uint32_t timeout_ms) {
  uint32_t previous_us = static_cast<uint32_t>(GetSystemTimeUs());
  uint64_t elapsed_us = 0;
  const uint64_t timeout_us = static_cast<uint64_t>(timeout_ms) * 1000;
  while (true) {
    uint8_t value = 0;
    if (!ReadRegister(reg, value)) {
      return false;
    }
    if ((value & mask) == 0) {
      return true;
    }
    // NRF52 micros() 为 32 位，累计相邻轮询差值以处理计时回绕。
    const uint32_t now_us = static_cast<uint32_t>(GetSystemTimeUs());
    elapsed_us += static_cast<uint32_t>(now_us - previous_us);
    previous_us = now_us;
    if (elapsed_us >= timeout_us) {
      return false;
    }
    const uint64_t remaining_ms = (timeout_us - elapsed_us + 999) / 1000;
    DelayMs(static_cast<uint32_t>(remaining_ms < 10 ? remaining_ms : 10));
  }
}

bool Bq2589x::ResetRegisters(uint32_t timeout_ms) {
  return UpdateRegisterBits(Register::kReg14, 0x80, 0x80) &&
         WaitForClear(Register::kReg14, 0x80, timeout_ms);
}

bool Bq2589x::IsRegisterResetActive(bool& active) {
  return ReadFlag(Register::kReg14, 0x80, active);
}

bool Bq2589x::SetHighImpedanceEnabled(bool enabled) {
  return UpdateRegisterBits(Register::kReg00, 0x80, enabled ? 0x80 : 0x00);
}

bool Bq2589x::GetHighImpedanceEnabled(bool& enabled) {
  return ReadFlag(Register::kReg00, 0x80, enabled);
}

bool Bq2589x::SetIlimPinEnabled(bool enabled) {
  return HasFeature(Feature::kIlimPin) &&
         UpdateRegisterBits(Register::kReg00, 0x40, enabled ? 0x40 : 0x00);
}

bool Bq2589x::GetIlimPinEnabled(bool& enabled) {
  return HasFeature(Feature::kIlimPin) &&
         ReadFlag(Register::kReg00, 0x40, enabled);
}

bool Bq2589x::SetInputCurrentOptimizerEnabled(bool enabled) {
  return HasFeature(Feature::kInputCurrentOptimizer) &&
         UpdateRegisterBits(Register::kReg02, 0x10, enabled ? 0x10 : 0x00);
}

bool Bq2589x::GetInputCurrentOptimizerEnabled(bool& enabled) {
  return HasFeature(Feature::kInputCurrentOptimizer) &&
         ReadFlag(Register::kReg02, 0x10, enabled);
}

bool Bq2589x::SetAutomaticInputDetectionEnabled(bool enabled) {
  return UpdateRegisterBits(Register::kReg02, 0x01, enabled ? 0x01 : 0x00);
}

bool Bq2589x::GetAutomaticInputDetectionEnabled(bool& enabled) {
  return ReadFlag(Register::kReg02, 0x01, enabled);
}

bool Bq2589x::SetBatteryLoadEnabled(bool enabled) {
  return HasFeature(Feature::kBatteryLoad) &&
         UpdateRegisterBits(Register::kReg03, 0x80, enabled ? 0x80 : 0x00);
}

bool Bq2589x::GetBatteryLoadEnabled(bool& enabled) {
  return HasFeature(Feature::kBatteryLoad) &&
         ReadFlag(Register::kReg03, 0x80, enabled);
}

bool Bq2589x::SetOtgEnabled(bool enabled) {
  return HasFeature(Feature::kOtg) &&
         UpdateRegisterBits(Register::kReg03, 0x20, enabled ? 0x20 : 0x00);
}

bool Bq2589x::GetOtgEnabled(bool& enabled) {
  return HasFeature(Feature::kOtg) && ReadFlag(Register::kReg03, 0x20, enabled);
}

bool Bq2589x::SetChargeEnabled(bool enabled) {
  return UpdateRegisterBits(Register::kReg03, 0x10, enabled ? 0x10 : 0x00);
}

bool Bq2589x::GetChargeEnabled(bool& enabled) {
  return ReadFlag(Register::kReg03, 0x10, enabled);
}

bool Bq2589x::SetCurrentPulseControlEnabled(bool enabled) {
  return HasFeature(Feature::kCurrentPulseControl) &&
         UpdateRegisterBits(Register::kReg04, 0x80, enabled ? 0x80 : 0x00);
}

bool Bq2589x::GetCurrentPulseControlEnabled(bool& enabled) {
  return HasFeature(Feature::kCurrentPulseControl) &&
         ReadFlag(Register::kReg04, 0x80, enabled);
}

bool Bq2589x::SetChargeTerminationEnabled(bool enabled) {
  return UpdateRegisterBits(Register::kReg07, 0x80, enabled ? 0x80 : 0x00);
}

bool Bq2589x::GetChargeTerminationEnabled(bool& enabled) {
  return ReadFlag(Register::kReg07, 0x80, enabled);
}

bool Bq2589x::SetStatPinEnabled(bool enabled) {
  return UpdateRegisterBits(Register::kReg07, 0x40, !enabled ? 0x40 : 0x00);
}

bool Bq2589x::GetStatPinEnabled(bool& enabled) {
  return ReadFlag(Register::kReg07, 0x40, enabled, true);
}

bool Bq2589x::SetSafetyTimerEnabled(bool enabled) {
  return UpdateRegisterBits(Register::kReg07, 0x08, enabled ? 0x08 : 0x00);
}

bool Bq2589x::GetSafetyTimerEnabled(bool& enabled) {
  return ReadFlag(Register::kReg07, 0x08, enabled);
}

bool Bq2589x::SetSafetyTimerSlowdownEnabled(bool enabled) {
  return UpdateRegisterBits(Register::kReg09, 0x40, enabled ? 0x40 : 0x00);
}

bool Bq2589x::GetSafetyTimerSlowdownEnabled(bool& enabled) {
  return ReadFlag(Register::kReg09, 0x40, enabled);
}

bool Bq2589x::SetBatfetEnabled(bool enabled) {
  return HasFeature(Feature::kBatfetControl) &&
         UpdateRegisterBits(Register::kReg09, 0x20, !enabled ? 0x20 : 0x00);
}

bool Bq2589x::GetBatfetEnabled(bool& enabled) {
  return ReadFlag(Register::kReg09, 0x20, enabled, true);
}

bool Bq2589x::SetBatfetResetEnabled(bool enabled) {
  return HasFeature(Feature::kBatfetReset) &&
         UpdateRegisterBits(Register::kReg09, 0x04, enabled ? 0x04 : 0x00);
}

bool Bq2589x::GetBatfetResetEnabled(bool& enabled) {
  return HasFeature(Feature::kBatfetReset) &&
         ReadFlag(Register::kReg09, 0x04, enabled);
}

bool Bq2589x::SetBoostPfmEnabled(bool enabled) {
  return HasFeature(Feature::kBoostPfm) &&
         UpdateRegisterBits(Register::kReg0a, 0x08, !enabled ? 0x08 : 0x00);
}

bool Bq2589x::GetBoostPfmEnabled(bool& enabled) {
  return HasFeature(Feature::kBoostPfm) &&
         ReadFlag(Register::kReg0a, 0x08, enabled, true);
}

bool Bq2589x::SetBoostHotThreshold(BoostHotThreshold threshold) {
  const auto code = static_cast<uint8_t>(threshold);
  if (!HasFeature(Feature::kBoostTemperatureThresholds) || code > 3) {
    return false;
  }
  return UpdateRegisterBits(
      Register::kReg01, 0xC0, static_cast<uint8_t>(code << 6));
}

bool Bq2589x::GetBoostHotThreshold(BoostHotThreshold& threshold) {
  uint8_t code = 0;
  if (!HasFeature(Feature::kBoostTemperatureThresholds) ||
      !ReadField(Register::kReg01, 0xC0, 6, code)) {
    return false;
  }
  threshold = static_cast<BoostHotThreshold>(code);
  return true;
}

bool Bq2589x::SetBoostColdThreshold(BoostColdThreshold threshold) {
  const auto code = static_cast<uint8_t>(threshold);
  if (!HasFeature(Feature::kBoostTemperatureThresholds) || code > 1) {
    return false;
  }
  return UpdateRegisterBits(
      Register::kReg01, 0x20, static_cast<uint8_t>(code << 5));
}

bool Bq2589x::GetBoostColdThreshold(BoostColdThreshold& threshold) {
  uint8_t code = 0;
  if (!HasFeature(Feature::kBoostTemperatureThresholds) ||
      !ReadField(Register::kReg01, 0x20, 5, code)) {
    return false;
  }
  threshold = static_cast<BoostColdThreshold>(code);
  return true;
}

bool Bq2589x::SetDpDmDac(bool dplus, DpDmVoltage voltage) {
  return HasFeature(Feature::kDpDmDac) &&
         model_driver_->SetDpDmDac(*this, dplus, voltage);
}

bool Bq2589x::IsDpDmDacReady() {
  bool detection_active = false;
  ChipStatus status;
  return IsInputDetectionActive(detection_active) && !detection_active &&
         GetChipStatus(status) && status.power_good &&
         status.vbus_status != VbusStatus::kNoInput &&
         status.vbus_status != VbusStatus::kOtg;
}

bool Bq2589x::SetDpDac(DpDmVoltage voltage) {
  return SetDpDmDac(true, voltage);
}

bool Bq2589x::GetDpDac(DpDmVoltage& voltage) {
  return HasFeature(Feature::kDpDmDac) &&
         model_driver_->GetDpDmDac(*this, true, voltage);
}

bool Bq2589x::SetDmDac(DpDmVoltage voltage) {
  return SetDpDmDac(false, voltage);
}

bool Bq2589x::GetDmDac(DpDmVoltage& voltage) {
  return HasFeature(Feature::kDpDmDac) &&
         model_driver_->GetDpDmDac(*this, false, voltage);
}

bool Bq2589x::Set12VoltDetectionEnabled(bool enabled) {
  return HasFeature(Feature::k12VoltDetection) &&
         UpdateRegisterBits(Register::kReg01, 0x02, enabled ? 0x02 : 0x00);
}

bool Bq2589x::Get12VoltDetectionEnabled(bool& enabled) {
  return HasFeature(Feature::k12VoltDetection) &&
         ReadFlag(Register::kReg01, 0x02, enabled);
}

bool Bq2589x::SetHvdcpEnabled(bool enabled) {
  return HasFeature(Feature::kHvdcp) &&
         UpdateRegisterBits(Register::kReg02, 0x08, enabled ? 0x08 : 0x00);
}

bool Bq2589x::GetHvdcpEnabled(bool& enabled) {
  return HasFeature(Feature::kHvdcp) &&
         ReadFlag(Register::kReg02, 0x08, enabled);
}

bool Bq2589x::SetMaxChargeEnabled(bool enabled) {
  return HasFeature(Feature::kMaxCharge) &&
         UpdateRegisterBits(Register::kReg02, 0x04, enabled ? 0x04 : 0x00);
}

bool Bq2589x::GetMaxChargeEnabled(bool& enabled) {
  return HasFeature(Feature::kMaxCharge) &&
         ReadFlag(Register::kReg02, 0x04, enabled);
}

bool Bq2589x::SetDselForcedHigh(bool forced_high) {
  return HasFeature(Feature::kForceDsel) &&
         UpdateRegisterBits(Register::kReg03, 0x80, forced_high ? 0x80 : 0x00);
}

bool Bq2589x::GetDselForcedHigh(bool& forced_high) {
  return HasFeature(Feature::kForceDsel) &&
         ReadFlag(Register::kReg03, 0x80, forced_high);
}

bool Bq2589x::SetVokOtgEnabled(bool enabled) {
  return HasFeature(Feature::kVokOtg) &&
         UpdateRegisterBits(Register::kReg03, 0x80, enabled ? 0x80 : 0x00);
}

bool Bq2589x::GetVokOtgEnabled(bool& enabled) {
  return HasFeature(Feature::kVokOtg) &&
         ReadFlag(Register::kReg03, 0x80, enabled);
}

bool Bq2589x::SetAdcConversionMode(AdcConversionMode mode) {
  const auto code = static_cast<uint8_t>(mode);
  if (code > 1) {
    return false;
  }
  return UpdateRegisterBits(
      Register::kReg02, 0x40, static_cast<uint8_t>(code << 6));
}

bool Bq2589x::GetAdcConversionMode(AdcConversionMode& mode) {
  uint8_t code = 0;
  if (!ReadField(Register::kReg02, 0x40, 6, code)) {
    return false;
  }
  mode = static_cast<AdcConversionMode>(code);
  return true;
}

bool Bq2589x::SetJeitaLowTemperatureCurrent(
    JeitaLowTemperatureCurrent setting) {
  const auto code = static_cast<uint8_t>(setting);
  if (!HasFeature(Feature::kJeita) || code > 1) {
    return false;
  }
  return UpdateRegisterBits(
      Register::kReg07, 0x01, static_cast<uint8_t>(code << 0));
}

bool Bq2589x::GetJeitaLowTemperatureCurrent(
    JeitaLowTemperatureCurrent& setting) {
  uint8_t code = 0;
  if (!HasFeature(Feature::kJeita) ||
      !ReadField(Register::kReg07, 0x01, 0, code)) {
    return false;
  }
  setting = static_cast<JeitaLowTemperatureCurrent>(code);
  return true;
}

bool Bq2589x::SetJeitaHighTemperatureVoltage(
    JeitaHighTemperatureVoltage setting) {
  const auto code = static_cast<uint8_t>(setting);
  if (!HasFeature(Feature::kJeita) || code > 1) {
    return false;
  }
  return UpdateRegisterBits(
      Register::kReg09, 0x10, static_cast<uint8_t>(code << 4));
}

bool Bq2589x::GetJeitaHighTemperatureVoltage(
    JeitaHighTemperatureVoltage& setting) {
  uint8_t code = 0;
  if (!HasFeature(Feature::kJeita) ||
      !ReadField(Register::kReg09, 0x10, 4, code)) {
    return false;
  }
  setting = static_cast<JeitaHighTemperatureVoltage>(code);
  return true;
}

bool Bq2589x::SetBatfetTurnOffDelay(BatfetTurnOffDelay delay) {
  const auto code = static_cast<uint8_t>(delay);
  if (!HasFeature(Feature::kBatfetDelay) || code > 1) {
    return false;
  }
  return UpdateRegisterBits(
      Register::kReg09, 0x08, static_cast<uint8_t>(code << 3));
}

bool Bq2589x::GetBatfetTurnOffDelay(BatfetTurnOffDelay& delay) {
  uint8_t code = 0;
  if (!HasFeature(Feature::kBatfetDelay) ||
      !ReadField(Register::kReg09, 0x08, 3, code)) {
    return false;
  }
  delay = static_cast<BatfetTurnOffDelay>(code);
  return true;
}

bool Bq2589x::SetVindpmMode(VindpmMode mode) {
  const auto code = static_cast<uint8_t>(mode);
  if (code > 1) {
    return false;
  }
  return UpdateRegisterBits(
      Register::kReg0d, 0x80, static_cast<uint8_t>(code << 7));
}

bool Bq2589x::GetVindpmMode(VindpmMode& mode) {
  uint8_t code = 0;
  if (!ReadField(Register::kReg0d, 0x80, 7, code)) {
    return false;
  }
  mode = static_cast<VindpmMode>(code);
  return true;
}

bool Bq2589x::SetInputCurrentLimit(uint16_t current_ma) {
  return SetLinearField(Register::kReg00, 0x3F, 0, current_ma, 100, 3250, 50);
}

bool Bq2589x::GetInputCurrentLimit(uint16_t& current_ma) {
  return GetLinearField(Register::kReg00, 0x3F, 0, 100, 3250, 50, current_ma);
}

bool Bq2589x::SetInputVoltageLimitOffset(uint16_t offset_mv) {
  return IsSupported() &&
         model_driver_->SetInputVoltageLimitOffset(*this, offset_mv);
}

bool Bq2589x::GetInputVoltageLimitOffset(uint16_t& offset_mv) {
  return IsSupported() &&
         model_driver_->GetInputVoltageLimitOffset(*this, offset_mv);
}

bool Bq2589x::SetSystemMinimumVoltage(uint16_t voltage_mv) {
  return SetLinearField(Register::kReg03, 0x0E, 1, voltage_mv, 3000, 3700, 100);
}

bool Bq2589x::GetSystemMinimumVoltage(uint16_t& voltage_mv) {
  return GetLinearField(Register::kReg03, 0x0E, 1, 3000, 3700, 100, voltage_mv);
}

bool Bq2589x::SetFastChargeCurrentLimit(uint16_t current_ma) {
  return IsSupported() &&
         model_driver_->SetFastChargeCurrentLimit(*this, current_ma);
}

bool Bq2589x::GetFastChargeCurrentLimit(uint16_t& current_ma) {
  return IsSupported() &&
         model_driver_->GetFastChargeCurrentLimit(*this, current_ma);
}

bool Bq2589x::SetPrechargeCurrentLimit(uint16_t current_ma) {
  return SetLinearField(Register::kReg05, 0xF0, 4, current_ma, 64, 1024, 64);
}

bool Bq2589x::GetPrechargeCurrentLimit(uint16_t& current_ma) {
  return GetLinearField(Register::kReg05, 0xF0, 4, 64, 1024, 64, current_ma);
}

bool Bq2589x::SetTerminationCurrentLimit(uint16_t current_ma) {
  return SetLinearField(Register::kReg05, 0x0F, 0, current_ma, 64, 1024, 64);
}

bool Bq2589x::GetTerminationCurrentLimit(uint16_t& current_ma) {
  return GetLinearField(Register::kReg05, 0x0F, 0, 64, 1024, 64, current_ma);
}

bool Bq2589x::SetChargeVoltageLimit(uint16_t voltage_mv) {
  return SetLinearField(Register::kReg06, 0xFC, 2, voltage_mv, 3840, 4608, 16);
}

bool Bq2589x::GetChargeVoltageLimit(uint16_t& voltage_mv) {
  return GetLinearField(Register::kReg06, 0xFC, 2, 3840, 4608, 16, voltage_mv);
}

bool Bq2589x::SetIrCompensationResistance(uint16_t resistance_mohm) {
  return HasFeature(Feature::kIrCompensation) &&
         SetLinearField(Register::kReg08, 0xE0, 5, resistance_mohm, 0, 140, 20);
}

bool Bq2589x::GetIrCompensationResistance(uint16_t& resistance_mohm) {
  return HasFeature(Feature::kIrCompensation) &&
         GetLinearField(Register::kReg08, 0xE0, 5, 0, 140, 20, resistance_mohm);
}

bool Bq2589x::SetIrCompensationVoltageClamp(uint16_t voltage_mv) {
  return HasFeature(Feature::kIrCompensation) &&
         SetLinearField(Register::kReg08, 0x1C, 2, voltage_mv, 0, 224, 32);
}

bool Bq2589x::GetIrCompensationVoltageClamp(uint16_t& voltage_mv) {
  return HasFeature(Feature::kIrCompensation) &&
         GetLinearField(Register::kReg08, 0x1C, 2, 0, 224, 32, voltage_mv);
}

bool Bq2589x::SetBoostVoltage(uint16_t voltage_mv) {
  return HasFeature(Feature::kOtg) &&
         SetLinearField(Register::kReg0a, 0xF0, 4, voltage_mv, 4550, 5510, 64);
}

bool Bq2589x::GetBoostVoltage(uint16_t& voltage_mv) {
  return HasFeature(Feature::kOtg) &&
         GetLinearField(Register::kReg0a, 0xF0, 4, 4550, 5510, 64, voltage_mv);
}

bool Bq2589x::SetBoostMinimumBatteryVoltage(uint16_t voltage_mv) {
  return HasFeature(Feature::kBoostMinimumBatteryVoltage) &&
         SetTableField(Register::kReg03, 0x01, 0, voltage_mv,
             kBoostMinimumBatteryVoltagesMv,
             sizeof(kBoostMinimumBatteryVoltagesMv) /
                 sizeof(kBoostMinimumBatteryVoltagesMv[0]));
}

bool Bq2589x::GetBoostMinimumBatteryVoltage(uint16_t& voltage_mv) {
  return HasFeature(Feature::kBoostMinimumBatteryVoltage) &&
         GetTableField(Register::kReg03, 0x01, 0,
             kBoostMinimumBatteryVoltagesMv,
             sizeof(kBoostMinimumBatteryVoltagesMv) /
                 sizeof(kBoostMinimumBatteryVoltagesMv[0]),
             voltage_mv);
}

bool Bq2589x::SetPrechargeToFastChargeThreshold(uint16_t voltage_mv) {
  return SetTableField(Register::kReg06, 0x02, 1, voltage_mv,
      kPrechargeThresholdsMv,
      sizeof(kPrechargeThresholdsMv) / sizeof(kPrechargeThresholdsMv[0]));
}

bool Bq2589x::GetPrechargeToFastChargeThreshold(uint16_t& voltage_mv) {
  return GetTableField(Register::kReg06, 0x02, 1, kPrechargeThresholdsMv,
      sizeof(kPrechargeThresholdsMv) / sizeof(kPrechargeThresholdsMv[0]),
      voltage_mv);
}

bool Bq2589x::SetRechargeThresholdOffset(uint16_t offset_mv) {
  return SetTableField(Register::kReg06, 0x01, 0, offset_mv, kRechargeOffsetsMv,
      sizeof(kRechargeOffsetsMv) / sizeof(kRechargeOffsetsMv[0]));
}

bool Bq2589x::GetRechargeThresholdOffset(uint16_t& offset_mv) {
  return GetTableField(Register::kReg06, 0x01, 0, kRechargeOffsetsMv,
      sizeof(kRechargeOffsetsMv) / sizeof(kRechargeOffsetsMv[0]), offset_mv);
}

bool Bq2589x::SetWatchdogTimer(uint16_t timeout_s) {
  return SetTableField(Register::kReg07, 0x30, 4, timeout_s, kWatchdogTimeoutsS,
      sizeof(kWatchdogTimeoutsS) / sizeof(kWatchdogTimeoutsS[0]));
}

bool Bq2589x::GetWatchdogTimer(uint16_t& timeout_s) {
  return GetTableField(Register::kReg07, 0x30, 4, kWatchdogTimeoutsS,
      sizeof(kWatchdogTimeoutsS) / sizeof(kWatchdogTimeoutsS[0]), timeout_s);
}

bool Bq2589x::SetFastChargeTimer(uint16_t duration_hours) {
  return SetTableField(Register::kReg07, 0x06, 1, duration_hours,
      kFastChargeTimersHours,
      sizeof(kFastChargeTimersHours) / sizeof(kFastChargeTimersHours[0]));
}

bool Bq2589x::GetFastChargeTimer(uint16_t& duration_hours) {
  return GetTableField(Register::kReg07, 0x06, 1, kFastChargeTimersHours,
      sizeof(kFastChargeTimersHours) / sizeof(kFastChargeTimersHours[0]),
      duration_hours);
}

bool Bq2589x::SetThermalRegulationThreshold(uint16_t temperature_c) {
  return SetTableField(Register::kReg08, 0x03, 0, temperature_c,
      kThermalThresholdsC,
      sizeof(kThermalThresholdsC) / sizeof(kThermalThresholdsC[0]));
}

bool Bq2589x::GetThermalRegulationThreshold(uint16_t& temperature_c) {
  return GetTableField(Register::kReg08, 0x03, 0, kThermalThresholdsC,
      sizeof(kThermalThresholdsC) / sizeof(kThermalThresholdsC[0]),
      temperature_c);
}

bool Bq2589x::SetBoostCurrentLimit(uint16_t current_ma) {
  return HasFeature(Feature::kBoostCurrentLimit) &&
         model_driver_->SetBoostCurrentLimit(*this, current_ma);
}

bool Bq2589x::GetBoostCurrentLimit(uint16_t& current_ma) {
  return HasFeature(Feature::kBoostCurrentLimit) &&
         model_driver_->GetBoostCurrentLimit(*this, current_ma);
}

bool Bq2589x::IsAdcConversionActive(bool& active) {
  return ReadFlag(Register::kReg02, 0x80, active);
}

bool Bq2589x::IsInputDetectionActive(bool& active) {
  return ReadFlag(Register::kReg02, 0x02, active);
}

bool Bq2589x::IsInputCurrentOptimizationComplete(bool& complete) {
  return HasFeature(Feature::kInputCurrentOptimizer) &&
         ReadFlag(Register::kReg14, 0x40, complete);
}

bool Bq2589x::IsThermalRegulationActive(bool& active) {
  return ReadFlag(Register::kReg0e, 0x80, active);
}

bool Bq2589x::IsVbusAttached(bool& attached) {
  return ReadFlag(Register::kReg11, 0x80, attached);
}

bool Bq2589x::GetBatteryVoltage(uint16_t& voltage_mv) {
  return GetLinearField(Register::kReg0e, 0x7F, 0, 2304, 4844, 20, voltage_mv);
}

bool Bq2589x::GetSystemVoltage(uint16_t& voltage_mv) {
  return GetLinearField(Register::kReg0f, 0x7F, 0, 2304, 4844, 20, voltage_mv);
}

bool Bq2589x::GetVbusVoltage(uint16_t& voltage_mv) {
  return GetLinearField(
      Register::kReg11, 0x7F, 0, 2600, 15300, 100, voltage_mv);
}

bool Bq2589x::GetChargeCurrent(uint16_t& current_ma) {
  return GetLinearField(Register::kReg12, 0x7F, 0, 0, 6350, 50, current_ma);
}

bool Bq2589x::StartAdcConversion() {
  uint8_t value = 0;
  if (!ReadRegister(Register::kReg02, value) || (value & 0xC2) != 0) {
    return false;
  }
  return UpdateRegisterBits(Register::kReg02, 0x80, 0x80);
}

bool Bq2589x::WaitForAdcConversion(uint32_t timeout_ms) {
  AdcConversionMode mode;
  if (!GetAdcConversionMode(mode) || mode != AdcConversionMode::kOneShot) {
    return false;
  }
  return WaitForClear(Register::kReg02, 0x80, timeout_ms);
}

bool Bq2589x::SetBoostFrequency(BoostFrequency frequency) {
  const auto code = static_cast<uint8_t>(frequency);
  if (code > 1) {
    return false;
  }
  bool otg_enabled = false;
  if (!GetOtgEnabled(otg_enabled) || otg_enabled) {
    return false;
  }
  return UpdateRegisterBits(
      Register::kReg02, 0x20, static_cast<uint8_t>(code << 5));
}

bool Bq2589x::GetBoostFrequency(BoostFrequency& frequency) {
  uint8_t code = 0;
  if (!HasFeature(Feature::kOtg) ||
      !ReadField(Register::kReg02, 0x20, 5, code)) {
    return false;
  }
  frequency = static_cast<BoostFrequency>(code);
  return true;
}

bool Bq2589x::ForceInputDetection() {
  bool active = false;
  if (!IsInputDetectionActive(active) || active) {
    return false;
  }
  return UpdateRegisterBits(Register::kReg02, 0x02, 0x02);
}

bool Bq2589x::ResetWatchdogTimer() {
  return UpdateRegisterBits(Register::kReg03, 0x40, 0x40);
}

bool Bq2589x::ForceInputCurrentOptimization() {
  bool enabled = false;
  if (!GetInputCurrentOptimizerEnabled(enabled) || !enabled) {
    return false;
  }
  return UpdateRegisterBits(Register::kReg09, 0x80, 0x80);
}

bool Bq2589x::EnterShipMode(BatfetTurnOffDelay delay) {
  const auto code = static_cast<uint8_t>(delay);
  if (!HasFeature(Feature::kBatfetControl) ||
      !HasFeature(Feature::kBatfetDelay) || code > 1) {
    return false;
  }
  return UpdateRegisterBits(
      Register::kReg09, 0x28, static_cast<uint8_t>(0x20 | (code << 3)));
}

bool Bq2589x::StartPumpx(bool increase) {
  bool enabled = false;
  PumpxStatus status;
  if (!GetCurrentPulseControlEnabled(enabled) || !enabled ||
      !GetPumpxStatus(status) || status.voltage_increase_active ||
      status.voltage_decrease_active) {
    return false;
  }
  return UpdateRegisterBits(Register::kReg09, 0x03, increase ? 0x02 : 0x01);
}

bool Bq2589x::StartPumpxVoltageIncrease() { return StartPumpx(true); }

bool Bq2589x::StartPumpxVoltageDecrease() { return StartPumpx(false); }

bool Bq2589x::GetPumpxStatus(PumpxStatus& status) {
  uint8_t value = 0;
  if (!HasFeature(Feature::kCurrentPulseControl) ||
      !ReadRegister(Register::kReg09, value)) {
    return false;
  }
  PumpxStatus result;
  result.voltage_increase_active = (value & 0x02) != 0;
  result.voltage_decrease_active = (value & 0x01) != 0;
  status = result;
  return true;
}

bool Bq2589x::GetChipStatus(ChipStatus& status) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kReg0b, value)) {
    return false;
  }
  status = model_driver_->DecodeChipStatus(value);
  return true;
}

bool Bq2589x::GetUsbSdpCurrentLimit(uint16_t& current_ma) {
  ChipStatus status;
  if (!HasFeature(Feature::kSdpStatus) || !GetChipStatus(status) ||
      !status.sdp_status_valid) {
    return false;
  }
  current_ma = status.usb_sdp_current_limit_ma;
  return true;
}

Bq2589x::FaultStatus Bq2589x::DecodeFaultStatus(uint8_t value) const {
  FaultStatus status;
  status.watchdog_expired = (value & 0x80) != 0;
  status.boost_fault_valid = HasFeature(Feature::kOtg);
  status.boost_fault = status.boost_fault_valid && (value & 0x40) != 0;
  status.charge_fault = static_cast<ChargeFault>((value >> 4) & 0x03);
  status.battery_overvoltage = (value & 0x08) != 0;
  const uint8_t ntc = value & 0x07;
  status.ntc_fault_valid = HasFeature(Feature::kTsAdc);
  status.ntc_fault = model_driver_->DecodeNtcFault(ntc);
  status.raw = value;
  return status;
}

bool Bq2589x::GetFaultStatus(FaultStatus& status) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kReg0c, value)) {
    return false;
  }
  status = DecodeFaultStatus(value);
  return true;
}

bool Bq2589x::GetFaultStatus(FaultStatus& latched, FaultStatus& current) {
  if (&latched == &current) {
    return false;
  }
  uint8_t latched_value = 0;
  uint8_t current_value = 0;
  // REG0C 禁止突发读取；不能拆成多个字段读取，否则会混合锁存值与当前值。
  if (!ReadRegister(Register::kReg0c, latched_value) ||
      !ReadRegister(Register::kReg0c, current_value)) {
    return false;
  }
  latched = DecodeFaultStatus(latched_value);
  current = DecodeFaultStatus(current_value);
  return true;
}

bool Bq2589x::SetAbsoluteVindpmThreshold(uint16_t voltage_mv) {
  if (voltage_mv < 3900 || voltage_mv > 15300) {
    return false;
  }
  // VINDPM 的最小有效阈值为 3900 mV，但编码偏移仍是 2600 mV。
  return UpdateRegisterBits(Register::kReg0d, 0xFF,
      static_cast<uint8_t>(0x80 | ((voltage_mv - 2600) / 100)));
}

bool Bq2589x::GetVindpmThreshold(uint16_t& voltage_mv) {
  uint16_t result = 0;
  if (!GetLinearField(Register::kReg0d, 0x7F, 0, 2600, 15300, 100, result) ||
      result < 3900) {
    return false;
  }
  voltage_mv = result;
  return true;
}

bool Bq2589x::GetTsVoltagePercentage(float& percentage) {
  uint8_t value = 0;
  if (!HasFeature(Feature::kTsAdc) ||
      !ReadField(Register::kReg10, 0x7F, 0, value)) {
    return false;
  }
  percentage = 21.0f + static_cast<float>(value) * 0.465f;
  return true;
}

bool Bq2589x::GetAdcMeasurements(AdcMeasurements& measurements) {
  if (!IsSupported()) {
    return false;
  }
  uint8_t data[5] = {};
  if (!model_driver_->ReadAdcRegisters(*bus_, data)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "BQ2589x ADC read failed\n");
    return false;
  }
  AdcMeasurements result;
  result.battery_voltage_mv = 2304 + (data[0] & 0x7F) * 20;
  result.system_voltage_mv = 2304 + (data[1] & 0x7F) * 20;
  result.ts_voltage_valid = HasFeature(Feature::kTsAdc);
  if (result.ts_voltage_valid) {
    result.ts_voltage_percentage = 21.0f + (data[2] & 0x7F) * 0.465f;
  }
  result.vbus_voltage_mv = 2600 + (data[3] & 0x7F) * 100;
  result.charge_current_ma = (data[4] & 0x7F) * 50;
  result.thermal_regulation_active = (data[0] & 0x80) != 0;
  result.vbus_attached = (data[3] & 0x80) != 0;
  measurements = result;
  return true;
}

bool Bq2589x::GetDpmStatus(DpmStatus& status) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kReg13, value)) {
    return false;
  }
  status = model_driver_->DecodeDpmStatus(value);
  return true;
}

}  // namespace cpp_bus_driver
