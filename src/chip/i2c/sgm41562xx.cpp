/*
 * @Description: SGM41562 系列电池充电管理芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-07-25 16:20:43
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
constexpr uint8_t kWatchdogMask = 0x60;
constexpr uint8_t kChargeTerminationEnableMask = 0x10;
constexpr uint8_t kSafetyTimerEnableMask = 0x08;
constexpr uint8_t kSafetyTimerSettingMask = 0x06;
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
constexpr uint8_t kSgm41562BChargeVoltageReset = 0xA3;
constexpr uint8_t kSgm41562BSystemVoltageReset = 0x37;
constexpr uint8_t kSgm41562SaChargeVoltageReset = 0x8D;
constexpr uint8_t kSgm41562SaSystemVoltageReset = 0x73;
constexpr uint8_t kSafetyTimerHours[] = {3, 5, 8, 12};
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

  uint8_t device_id = 0;
  if (!GetDeviceId(device_id)) {
    return false;
  }

  if (device_id != kDeviceIdSgm41562BAndSa &&
      device_id != kDeviceIdSgm41562A &&
      device_id != kDeviceIdSgm41562 &&
      device_id != kDeviceIdSgm41562S) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Unsupported SGM41562xx id: %#X\n", device_id);
    return false;
  }

  if (!ResetRegisters()) {
    return false;
  }

  const ChipType detected_chip_type = DetectChipType(device_id);
  if (detected_chip_type == ChipType::kUnknown) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Identify SGM41562xx failed (id: %#X)\n", device_id);
    return false;
  }

  chip_type_ = detected_chip_type;
  const bool extended_register_map =
      chip_type_ == ChipType::kSgm41562S ||
      chip_type_ == ChipType::kSgm41562Sa;
  const uint8_t* init_sequence =
      extended_register_map ? kInitSequenceS : kInitSequenceAb;
  const size_t init_sequence_size =
      extended_register_map ? sizeof(kInitSequenceS) : sizeof(kInitSequenceAb);
  if (!InitSequence(init_sequence, init_sequence_size)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "InitSequence failed\n");
    chip_type_ = ChipType::kUnknown;
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "Get %s id success (id: %#X)\n", ChipTypeToString(chip_type_),
      device_id);
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

bool Sgm41562xx::GetDeviceId(uint8_t& device_id) {
  uint8_t value = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kDeviceId), &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read device id failed\n");
    return false;
  }

  device_id = value;
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

Sgm41562xx::ChipType Sgm41562xx::DetectChipType(uint8_t device_id) {
  switch (device_id) {
    case kDeviceIdSgm41562A:
      return ChipType::kSgm41562A;
    case kDeviceIdSgm41562:
      return ChipType::kSgm41562;
    case kDeviceIdSgm41562S:
      return ChipType::kSgm41562S;
    case kDeviceIdSgm41562BAndSa:
      return DetectIdZeroChipType();
    default:
      return ChipType::kUnknown;
  }
}

Sgm41562xx::ChipType Sgm41562xx::DetectIdZeroChipType() {
  uint8_t charge_voltage_control = 0;
  uint8_t system_voltage_regulation = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kChargeVoltageControl),
          &charge_voltage_control) ||
      !bus_->Read(static_cast<uint8_t>(Cmd::kSystemVoltageRegulation),
          &system_voltage_regulation)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Read reset values failed\n");
    return ChipType::kUnknown;
  }

  // B和SA的设备ID相同，软件复位后通过寄存器默认值区分型号
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
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kChargeCurrentControl),
          &charge_current_control) ||
      !bus_->Write(static_cast<uint8_t>(Cmd::kChargeCurrentControl),
          static_cast<uint8_t>(charge_current_control | kRegisterResetMask))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Reset registers failed\n");
    return false;
  }

  DelayMs(10);
  return true;
}

bool Sgm41562xx::UpdateRegisterBits(
    Cmd cmd, uint8_t mask, uint8_t value) {
  uint8_t current_value = 0;
  if (!bus_->Read(static_cast<uint8_t>(cmd), &current_value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read register failed\n");
    return false;
  }

  const uint8_t new_value =
      static_cast<uint8_t>((current_value & ~mask) | (value & mask));
  if (!bus_->Write(static_cast<uint8_t>(cmd), new_value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write register failed\n");
    return false;
  }

  return true;
}

bool Sgm41562xx::ReadRegister(
    Cmd cmd, uint8_t& value, const char* name) {
  if (!bus_->Read(static_cast<uint8_t>(cmd), &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Read %s failed (command: %#X)\n",
        name == nullptr ? "unknown register" : name,
        static_cast<unsigned int>(static_cast<uint8_t>(cmd)));
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

bool Sgm41562xx::GetIrqStatus(IrqStatus& status) {
  if (!IsInitialized()) {
    return false;
  }

  uint8_t irq_status = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kFaultAndShippingControl),
          &irq_status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  ParseIrqStatus(irq_status, status);
  return true;
}

void Sgm41562xx::ParseIrqStatus(uint8_t irq_status, IrqStatus& status) {
  status.input_power_fault = (irq_status & 0x20) != 0;
  status.thermal_shutdown = (irq_status & 0x10) != 0;
  status.battery_overvoltage_fault = (irq_status & 0x08) != 0;
  status.safety_timer_expired = (irq_status & 0x04) != 0;
  status.ntc_hot = (irq_status & 0x02) != 0;
  status.ntc_cold = (irq_status & 0x01) != 0;
}

bool Sgm41562xx::SetChargeEnable(bool enable) {
  if (!IsInitialized()) {
    return false;
  }

  return UpdateRegisterBits(Cmd::kPowerOnConfiguration,
      kChargeDisableMask, enable ? 0x00 : kChargeDisableMask);
}

bool Sgm41562xx::GetChipStatus(ChipStatus& status) {
  if (!IsInitialized()) {
    return false;
  }

  uint8_t chip_status = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kSystemStatus), &chip_status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  ParseChipStatus(chip_status, status);
  return true;
}

void Sgm41562xx::ParseChipStatus(
    uint8_t chip_status, ChipStatus& status) {
  status.watchdog_expired = (chip_status & 0x80) != 0;
  status.charge_status =
      static_cast<ChargeStatus>((chip_status & 0x18) >> 3);
  status.power_path_management_active = (chip_status & 0x04) != 0;
  status.input_power_good = (chip_status & 0x02) != 0;
  status.thermal_regulation_active = (chip_status & 0x01) != 0;
}

bool Sgm41562xx::GetChargerConfig(ChargerConfig& config) {
  if (!IsInitialized()) {
    return false;
  }

  ChargerConfig new_config;
  if (!ReadInputConfig(new_config) ||
      !ReadChargeConfig(new_config) ||
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
  if (!ReadRegister(Cmd::kInputSourceControl,
          input_source_control, "REG00 input source control") ||
      !ReadRegister(Cmd::kPowerOnConfiguration,
          power_on_configuration, "REG01 power-on configuration") ||
      !ReadRegister(Cmd::kSystemStatus,
          system_status, "REG08 system status")) {
    return false;
  }

  config.charge_enabled =
      (power_on_configuration & kChargeDisableMask) == 0;
  config.high_impedance_enabled =
      (power_on_configuration & kHighImpedanceEnableMask) != 0;
  config.input_voltage_limit_mv =
      3880 + 80 * ((input_source_control >> 4) & 0x0F);

  if (HasExtendedRegisterMap()) {
    uint8_t extended_input_current_control = 0;
    if (!ReadRegister(Cmd::kExtendedInputCurrentControl,
            extended_input_current_control,
            "REG0C extended input current control")) {
      return false;
    }
    config.input_current_limit_ma =
        50 + 30 * ((extended_input_current_control >> 3) & 0x1F);
    config.input_overvoltage_threshold_mv =
        (system_status & kInputOvervoltageSelectMask) != 0 ? 19000 : 6000;
    config.pcb_overtemperature_protection_enabled =
        (system_status & kPcbProtectionDisableSMask) == 0;
  } else {
    config.input_current_limit_enabled =
        (system_status & kInputCurrentLimitReleaseMask) == 0;
    config.input_current_limit_ma =
        50 + 30 * (input_source_control & 0x0F);
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
  if (!ReadRegister(Cmd::kChargeCurrentControl,
          charge_current_control, "REG02 charge current control") ||
      !ReadRegister(Cmd::kDischargeTerminationCurrent,
          discharge_termination_current,
          "REG03 discharge and termination current") ||
      !ReadRegister(Cmd::kChargeVoltageControl,
          charge_voltage_control, "REG04 charge voltage control")) {
    return false;
  }

  const bool extended_register_map = HasExtendedRegisterMap();
  uint8_t fast_charge_current_code = charge_current_control & 0x3F;
  uint8_t extended_current_control = 0;
  if (extended_register_map) {
    fast_charge_current_code = charge_current_control & 0x7F;
    if (!ReadRegister(Cmd::kExtendedCurrentControl,
            extended_current_control,
            "REG0D extended current control")) {
      return false;
    }
  } else if (fast_charge_current_code > 56) {
    fast_charge_current_code = 56;
  }
  config.fast_charge_current_ma = 8 + 8 * fast_charge_current_code;

  uint16_t termination_current_ma =
      1 + 2 * (discharge_termination_current & 0x0F);
  if (extended_register_map &&
      (extended_current_control & 0x04) != 0) {
    termination_current_ma *= 6;
  }
  config.termination_current_ma = termination_current_ma;

  if (extended_register_map) {
    config.charge_voltage_limit_mv =
        3500 + 10 * ((charge_voltage_control >> 1) & 0x7F);
  } else {
    config.charge_voltage_limit_mv =
        3600 + 15 * ((charge_voltage_control >> 2) & 0x3F);
  }
  return true;
}

bool Sgm41562xx::ReadProtectionConfig(ChargerConfig& config) {
  uint8_t charge_timer_control = 0;
  uint8_t miscellaneous_control = 0;
  uint8_t system_voltage_regulation = 0;
  if (!ReadRegister(Cmd::kChargeTerminationTimerControl,
          charge_timer_control,
          "REG05 charge termination and timer control") ||
      !ReadRegister(Cmd::kMiscellaneousOperationControl,
          miscellaneous_control,
          "REG06 miscellaneous operation control") ||
      !ReadRegister(Cmd::kSystemVoltageRegulation,
          system_voltage_regulation,
          "REG07 system voltage regulation")) {
    return false;
  }

  const bool extended_register_map = HasExtendedRegisterMap();
  if (extended_register_map) {
    config.system_voltage_regulation_mv =
        3600 + 50 * (system_voltage_regulation & 0x1F);
    config.input_voltage_loop_enabled =
        (system_voltage_regulation &
            kInputVoltageLoopDisableSMask) == 0;
    config.thermal_regulation_threshold_c =
        60 + 20 * ((system_voltage_regulation >> 5) & 0x03);
  } else {
    config.system_voltage_regulation_mv =
        4200 + 50 * (system_voltage_regulation & 0x0F);
    config.pcb_overtemperature_protection_enabled =
        (system_voltage_regulation &
            kPcbProtectionDisableAbMask) == 0;
    config.input_voltage_loop_enabled =
        (system_voltage_regulation &
            kInputVoltageLoopDisableAbMask) == 0;
    config.thermal_regulation_threshold_c =
        60 + 20 * ((system_voltage_regulation >> 4) & 0x03);
  }

  const uint8_t watchdog_setting =
      (charge_timer_control & kWatchdogMask) >> 5;
  config.watchdog_enabled = watchdog_setting != 0;
  if (config.watchdog_enabled) {
    const uint16_t watchdog_base_s = extended_register_map ? 64 : 40;
    config.watchdog_timeout_s =
        watchdog_base_s << (watchdog_setting - 1);
  }
  config.charge_termination_enabled =
      (charge_timer_control & kChargeTerminationEnableMask) != 0;
  config.safety_timer_enabled =
      (charge_timer_control & kSafetyTimerEnableMask) != 0;
  const uint8_t safety_timer_setting =
      (charge_timer_control & kSafetyTimerSettingMask) >> 1;
  config.safety_timer_hours = kSafetyTimerHours[safety_timer_setting];
  config.safety_timer_extended_in_ppm =
      (miscellaneous_control & kSafetyTimerExtendedMask) != 0;
  config.ntc_enabled =
      (miscellaneous_control & kNtcEnableMask) != 0;
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

  return UpdateRegisterBits(Cmd::kMiscellaneousOperationControl,
      kShippingModeEnableMask,
      enable ? kShippingModeEnableMask : 0x00);
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

  return UpdateRegisterBits(Cmd::kFaultAndShippingControl,
      kShippingModeDelayMask, static_cast<uint8_t>(delay_value << 6));
}

}  // namespace cpp_bus_driver
