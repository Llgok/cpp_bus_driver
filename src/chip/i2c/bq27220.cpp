/*
 * @Description: BQ27220 单节电池 CEDV 电量计驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-07-08 09:32:19
 * @License: GPL 3.0
 */
#include "bq27220.h"

namespace cpp_bus_driver {
bool Bq27220::Init(int32_t freq_hz) {
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

  const uint16_t device_id = GetDeviceId();
  if (device_id != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get bq27220 device id failed (error id: %#X)\n", device_id);
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "Get bq27220 device id success (id: %#X)\n", device_id);
  return true;
}

bool Bq27220::Deinit(bool delete_bus) {
  bool result = true;

  if (!ChipI2cGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
    result = false;
  }

  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }

  return result;
}

uint16_t Bq27220::GetDeviceId() {
  uint16_t value = 0;
  if (!ReadControlSubcommand(ControlSubcommand::kDeviceNumber, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ReadControlSubcommand failed\n");
  }
  return value;
}

uint16_t Bq27220::GetFirmwareVersion() {
  uint16_t value = 0;
  if (!ReadControlSubcommand(ControlSubcommand::kFirmwareVersion, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ReadControlSubcommand failed\n");
  }
  return value;
}

uint16_t Bq27220::GetHardwareVersion() {
  uint16_t value = 0;
  if (!ReadControlSubcommand(ControlSubcommand::kHardwareVersion, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ReadControlSubcommand failed\n");
  }
  return value;
}

uint16_t Bq27220::GetDesignCapacity() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kDesignCapacity, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetVoltage() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kVoltage, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

int16_t Bq27220::GetCurrent() {
  int16_t value = 0;
  if (!ReadS16(Cmd::kCurrent, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadS16 failed\n");
  }
  return value;
}

int16_t Bq27220::GetAverageCurrent() {
  int16_t value = 0;
  if (!ReadS16(Cmd::kAverageCurrent, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadS16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetRemainingCapacity() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kRemainingCapacity, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetFullChargeCapacity() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kFullChargeCapacity, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

int16_t Bq27220::GetAtRate() {
  int16_t value = 0;
  if (!ReadS16(Cmd::kAtRate, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadS16 failed\n");
  }
  return value;
}

bool Bq27220::SetAtRate(int16_t rate) {
  if (!WriteU16(Cmd::kAtRate, static_cast<uint16_t>(rate))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteU16 failed\n");
    return false;
  }
  return true;
}

uint16_t Bq27220::GetAtRateTimeToEmpty() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kAtRateTimeToEmpty, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetTemperatureRaw() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kTemperature, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

float Bq27220::GetTemperatureKelvin() { return GetTemperatureRaw() * 0.1f; }

float Bq27220::GetTemperatureCelsius() {
  return GetTemperatureKelvin() - 273.15f;
}

bool Bq27220::SetTemperatureMode(TemperatureMode mode) {
  if (!Unseal()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Unseal failed\n");
    return false;
  }

  uint16_t operation_config = 0;
  if (!ReadDataMemory(
          DataMemoryAddress::kOperationConfigA, &operation_config)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ReadDataMemory failed\n");
    return false;
  }

  switch (mode) {
    case TemperatureMode::kInternal:
      operation_config &= static_cast<uint16_t>(~0x8100);
      break;
    case TemperatureMode::kExternalNtc:
      operation_config &= static_cast<uint16_t>(~0x0100);
      operation_config |= 0x8000;
      break;
  }

  if (!WriteDataMemory(
          DataMemoryAddress::kOperationConfigA, operation_config)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WriteDataMemory failed\n");
    return false;
  }
  return true;
}

bool Bq27220::GetBatteryStatus(BatteryStatus& status) {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kBatteryStatus, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
    return false;
  }
  status.flag.discharging = value & (1U << 0);
  status.flag.system_down = value & (1U << 1);
  status.flag.terminate_discharge_alarm = value & (1U << 2);
  status.flag.battery_present = value & (1U << 3);
  status.flag.authentication_good = value & (1U << 4);
  status.flag.open_circuit_voltage_good = value & (1U << 5);
  status.flag.terminate_charge_alarm = value & (1U << 6);
  status.flag.charge_inhibit = value & (1U << 8);
  status.flag.full_charged = value & (1U << 9);
  status.flag.over_temperature_discharge = value & (1U << 10);
  status.flag.over_temperature_charge = value & (1U << 11);
  status.flag.sleep_mode = value & (1U << 12);
  status.flag.open_circuit_voltage_failed = value & (1U << 13);
  status.flag.open_circuit_voltage_complete = value & (1U << 14);
  status.flag.full_discharged = value & (1U << 15);
  return true;
}

bool Bq27220::GetOperationStatus(OperationStatus& status) {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kOperationStatus, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
    return false;
  }
  status.flag.calibration_mode = value & (1U << 0);
  status.security_mode_bits = (value >> 1) & 0x03;
  status.security = static_cast<SecurityMode>(status.security_mode_bits);
  status.flag.edv2_reached = value & (1U << 3);
  status.flag.valid_discharge_qualified = value & (1U << 4);
  status.flag.initialization_complete = value & (1U << 5);
  status.flag.smoothing_active = value & (1U << 6);
  status.flag.battery_trip_point_interrupt = value & (1U << 7);
  status.flag.config_update_mode = value & (1U << 10);
  return true;
}

bool Bq27220::SetDesignCapacity(uint16_t capacity) {
  if (!WriteDataMemory(DataMemoryAddress::kDesignCapacity, capacity)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WriteDataMemory failed\n");
    return false;
  }
  return true;
}

uint16_t Bq27220::GetTimeToEmpty() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kTimeToEmpty, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetTimeToFull() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kTimeToFull, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

int16_t Bq27220::GetStandbyCurrent() {
  int16_t value = 0;
  if (!ReadS16(Cmd::kStandbyCurrent, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadS16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetStandbyTimeToEmpty() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kStandbyTimeToEmpty, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

int16_t Bq27220::GetMaxLoadCurrent() {
  int16_t value = 0;
  if (!ReadS16(Cmd::kMaxLoadCurrent, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadS16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetMaxLoadTimeToEmpty() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kMaxLoadTimeToEmpty, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

int16_t Bq27220::GetRawCoulombCount() {
  int16_t value = 0;
  if (!ReadS16(Cmd::kRawCoulombCount, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadS16 failed\n");
  }
  return value;
}

int16_t Bq27220::GetAveragePower() {
  int16_t value = 0;
  if (!ReadS16(Cmd::kAveragePower, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadS16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetChipTemperatureRaw() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kInternalTemperature, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

float Bq27220::GetChipTemperatureKelvin() {
  return GetChipTemperatureRaw() * 0.1f;
}

float Bq27220::GetChipTemperatureCelsius() {
  return GetChipTemperatureKelvin() - 273.15f;
}

uint16_t Bq27220::GetCycleCount() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kCycleCount, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetStatusOfCharge() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kStatusOfCharge, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetStatusOfHealth() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kStatusOfHealth, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetChargingVoltage() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kChargingVoltage, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetChargingCurrent() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kChargingCurrent, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

bool Bq27220::SetBtpDischargeThreshold(uint16_t threshold_mah) {
  if (!WriteU16(Cmd::kBtpDischargeSet, threshold_mah)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteU16 failed\n");
    return false;
  }
  return true;
}

bool Bq27220::SetBtpChargeThreshold(uint16_t threshold_mah) {
  if (!WriteU16(Cmd::kBtpChargeSet, threshold_mah)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteU16 failed\n");
    return false;
  }
  return true;
}

uint16_t Bq27220::GetAnalogCount() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kAnalogCount, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

int16_t Bq27220::GetRawCurrent() {
  int16_t value = 0;
  if (!ReadS16(Cmd::kRawCurrent, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadS16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetRawVoltage() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kRawVoltage, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

uint16_t Bq27220::GetRawInternalTemperature() {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kRawInternalTemperature, &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
  }
  return value;
}

bool Bq27220::SetSleepCurrentThreshold(uint16_t threshold) {
  if (threshold > 100) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range\n");
    threshold = 100;
  }

  if (!WriteDataMemory(DataMemoryAddress::kSleepCurrent, threshold)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WriteDataMemory failed\n");
    return false;
  }
  return true;
}

bool Bq27220::SendControlSubcommand(ControlSubcommand subcommand) {
  if (!WriteU16(Cmd::kControl, static_cast<uint16_t>(subcommand))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteU16 failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ReadControlSubcommand(
    ControlSubcommand subcommand, uint16_t* value) {
  if (value == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (!SendControlSubcommand(subcommand)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  DelayMs(15);
  if (!ReadU16(Cmd::kMacData, value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
    return false;
  }
  return true;
}

bool Bq27220::Seal() {
  if (!SendControlSubcommand(ControlSubcommand::kSeal)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  DelayMs(10);

  OperationStatus status;
  if (!GetOperationStatus(status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GetOperationStatus failed\n");
    return false;
  }
  if (status.security != SecurityMode::kSealed) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Seal failed\n");
    return false;
  }
  return true;
}

bool Bq27220::Unseal() {
  OperationStatus status;
  if (GetOperationStatus(status) &&
      (status.security == SecurityMode::kUnsealed ||
          status.security == SecurityMode::kFullAccess)) {
    return true;
  }

  if (!WriteU16(Cmd::kControl, kUnsealKey1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteU16 failed\n");
    return false;
  }
  DelayMs(10);
  if (!WriteU16(Cmd::kControl, kUnsealKey2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteU16 failed\n");
    return false;
  }
  DelayMs(10);

  if (!GetOperationStatus(status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GetOperationStatus failed\n");
    return false;
  }
  if (status.security != SecurityMode::kUnsealed &&
      status.security != SecurityMode::kFullAccess) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Unseal failed\n");
    return false;
  }
  return true;
}

bool Bq27220::FullAccess() {
  OperationStatus status;
  if (GetOperationStatus(status) &&
      status.security == SecurityMode::kFullAccess) {
    return true;
  }

  if (!Unseal()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Unseal failed\n");
    return false;
  }
  if (!WriteU16(Cmd::kControl, kFullAccessKey)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteU16 failed\n");
    return false;
  }
  DelayMs(10);
  if (!WriteU16(Cmd::kControl, kFullAccessKey)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteU16 failed\n");
    return false;
  }
  DelayMs(10);

  if (!GetOperationStatus(status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GetOperationStatus failed\n");
    return false;
  }
  if (status.security != SecurityMode::kFullAccess) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "FullAccess failed\n");
    return false;
  }
  return true;
}

bool Bq27220::Reset() {
  if (!SendControlSubcommand(ControlSubcommand::kReset)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  DelayMs(100);
  if (GetDeviceId() != kDeviceId) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Reset failed\n");
    return false;
  }
  return true;
}

bool Bq27220::SetBatteryInserted(bool inserted) {
  if (!SendControlSubcommand(inserted ? ControlSubcommand::kBatteryInsert
                                      : ControlSubcommand::kBatteryRemove)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  return true;
}

bool Bq27220::SetBatteryProfile(uint8_t profile) {
  if (profile < 1 || profile > 6) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range\n");
    return false;
  }

  const ControlSubcommand subcommands[] = {
      ControlSubcommand::kSetProfile1,
      ControlSubcommand::kSetProfile2,
      ControlSubcommand::kSetProfile3,
      ControlSubcommand::kSetProfile4,
      ControlSubcommand::kSetProfile5,
      ControlSubcommand::kSetProfile6,
  };
  if (!SendControlSubcommand(subcommands[profile - 1])) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  return true;
}

bool Bq27220::EnterCalibration() {
  if (!SendControlSubcommand(ControlSubcommand::kEnterCalibration)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ExitCalibration() {
  if (!SendControlSubcommand(ControlSubcommand::kExitCalibration)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ToggleCalibration() {
  if (!SendControlSubcommand(ControlSubcommand::kCalibrationToggle)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ReadDataMemory(DataMemoryAddress address, uint16_t* value) {
  if (!ReadDataMemory(static_cast<uint16_t>(address), value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ReadDataMemory failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ReadDataMemory(DataMemoryAddress address, uint8_t* value) {
  if (!ReadDataMemory(static_cast<uint16_t>(address), value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ReadDataMemory failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ReadDataMemory(uint16_t address, uint16_t* value) {
  if (value == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t data[2] = {};
  if (!ReadDataMemoryBytes(address, data, 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ReadDataMemoryBytes failed\n");
    return false;
  }
  *value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  return true;
}

bool Bq27220::ReadDataMemory(uint16_t address, uint8_t* value) {
  if (value == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (!ReadDataMemoryBytes(address, value, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ReadDataMemoryBytes failed\n");
    return false;
  }
  return true;
}

bool Bq27220::WriteDataMemory(DataMemoryAddress address, uint16_t value) {
  if (!WriteDataMemory(static_cast<uint16_t>(address), value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WriteDataMemory failed\n");
    return false;
  }
  return true;
}

bool Bq27220::WriteDataMemory(DataMemoryAddress address, uint8_t value) {
  if (!WriteDataMemory(static_cast<uint16_t>(address), value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WriteDataMemory failed\n");
    return false;
  }
  return true;
}

bool Bq27220::WriteDataMemory(uint16_t address, uint16_t value) {
  const uint8_t data[2] = {
      static_cast<uint8_t>(value >> 8),
      static_cast<uint8_t>(value),
  };
  if (!WriteDataMemoryBytes(address, data, 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WriteDataMemoryBytes failed\n");
    return false;
  }
  return true;
}

bool Bq27220::WriteDataMemory(uint16_t address, uint8_t value) {
  if (!WriteDataMemoryBytes(address, &value, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WriteDataMemoryBytes failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ApplyBatteryProfile(
    const CedvProfile& profile, const GaugingConfig& config) {
  if (!EnterConfigUpdate()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "EnterConfigUpdate failed\n");
    return false;
  }

  bool result = true;
  result &=
      WriteDataMemory(DataMemoryAddress::kGaugingConfiguration,
          config.raw_value);
  result &= WriteDataMemory(
      DataMemoryAddress::kFullChargeCapacity, profile.full_charge_capacity);
  result &= WriteDataMemory(
      DataMemoryAddress::kDesignCapacity, profile.design_capacity);
  result &= WriteDataMemory(DataMemoryAddress::kNearFull, profile.near_full);
  result &= WriteDataMemory(
      DataMemoryAddress::kSelfDischargeRate, profile.self_discharge_rate);
  result &= WriteDataMemory(
      DataMemoryAddress::kReserveCapacity, profile.reserve_capacity);
  result &= WriteDataMemory(DataMemoryAddress::kEmf, profile.emf);
  result &= WriteDataMemory(DataMemoryAddress::kC0, profile.c0);
  result &= WriteDataMemory(DataMemoryAddress::kR0, profile.r0);
  result &= WriteDataMemory(DataMemoryAddress::kT0, profile.t0);
  result &= WriteDataMemory(DataMemoryAddress::kR1, profile.r1);
  result &= WriteDataMemory(DataMemoryAddress::kTc, profile.tc);
  result &= WriteDataMemory(DataMemoryAddress::kC1, profile.c1);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod0, profile.dod0);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod10, profile.dod10);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod20, profile.dod20);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod30, profile.dod30);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod40, profile.dod40);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod50, profile.dod50);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod60, profile.dod60);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod70, profile.dod70);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod80, profile.dod80);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod90, profile.dod90);
  result &= WriteDataMemory(DataMemoryAddress::kVoltageDod100, profile.dod100);
  result &= WriteDataMemory(DataMemoryAddress::kFixedEdv0, profile.edv0);
  result &= WriteDataMemory(DataMemoryAddress::kFixedEdv1, profile.edv1);
  result &= WriteDataMemory(DataMemoryAddress::kFixedEdv2, profile.edv2);

  result &= ExitConfigUpdate(true);
  if (!result) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "ApplyBatteryProfile failed\n");
  }
  return result;
}

bool Bq27220::ApplyBatteryProfileIfNeeded(
    const CedvProfile& profile, const GaugingConfig& config) {
  if (!Unseal()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Unseal failed\n");
    return false;
  }

  uint16_t design_capacity = 0;
  uint16_t emf = 0;
  uint16_t t0 = 0;
  uint16_t dod20 = 0;

  const bool read_ok =
      ReadDataMemory(DataMemoryAddress::kDesignCapacity, &design_capacity) &&
      ReadDataMemory(DataMemoryAddress::kEmf, &emf) &&
      ReadDataMemory(DataMemoryAddress::kT0, &t0) &&
      ReadDataMemory(DataMemoryAddress::kVoltageDod20, &dod20);

  if (read_ok && design_capacity == profile.design_capacity &&
      emf == profile.emf && t0 == profile.t0 && dod20 == profile.dod20) {
    return true;
  }

  if (!ApplyBatteryProfile(profile, config)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ApplyBatteryProfile failed\n");
    return false;
  }
  return true;
}

bool Bq27220::EnterConfigUpdate() {
  if (!Unseal()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Unseal failed\n");
    return false;
  }

  if (!SendControlSubcommand(ControlSubcommand::kEnterConfigUpdate)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  DelayMs(10);
  if (!WaitConfigUpdate(true)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WaitConfigUpdate failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ExitConfigUpdate(bool reinit) {
  if (!SendControlSubcommand(reinit ? ControlSubcommand::kExitConfigUpdateReinit
                                    : ControlSubcommand::kExitConfigUpdate)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SendControlSubcommand failed\n");
    return false;
  }
  DelayMs(10);
  if (!WaitConfigUpdate(false)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WaitConfigUpdate failed\n");
    return false;
  }
  return true;
}

bool Bq27220::WaitConfigUpdate(bool enabled, uint32_t timeout_ms) {
  uint32_t elapsed_ms = 0;
  while (elapsed_ms <= timeout_ms) {
    OperationStatus status;
    if (GetOperationStatus(status) &&
        status.flag.config_update_mode == enabled) {
      return true;
    }
    DelayMs(10);
    elapsed_ms += 10;
  }

  LogMessage(LogLevel::kError, __FILE__, __LINE__, "WaitConfigUpdate timeout\n");
  return false;
}

bool Bq27220::ReadU16(Cmd cmd, uint16_t* value) {
  if (value == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer[2] = {};
  if (!bus_->Read(static_cast<uint8_t>(cmd), buffer, 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  *value = (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
  return true;
}

bool Bq27220::WriteU16(Cmd cmd, uint16_t value) {
  if (!bus_->Write(static_cast<uint8_t>(cmd), value, Endian::kLittle)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ReadS16(Cmd cmd, int16_t* value) {
  uint16_t raw = 0;
  if (!ReadU16(cmd, &raw)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadU16 failed\n");
    return false;
  }
  *value = static_cast<int16_t>(raw);
  return true;
}

bool Bq27220::WriteDataMemoryBytes(
    uint16_t address, const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > 32) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid argument\n");
    return false;
  }

  bool entered_config_update = false;
  OperationStatus status;
  if (!GetOperationStatus(status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GetOperationStatus failed\n");
    return false;
  }
  if (!status.flag.config_update_mode) {
    if (!EnterConfigUpdate()) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "EnterConfigUpdate failed\n");
      return false;
    }
    entered_config_update = true;
  }

  uint8_t buffer[34] = {
      static_cast<uint8_t>(address),
      static_cast<uint8_t>(address >> 8),
  };
  std::memcpy(&buffer[2], data, length);

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kSelectSubclass), buffer, length + 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    if (entered_config_update) {
      ExitConfigUpdate(false);
    }
    return false;
  }
  DelayMs(10);

  const uint8_t checksum = CalcChecksum(buffer, length + 2);
  const uint8_t checksum_buffer[2] = {
      checksum,
      static_cast<uint8_t>(length + 4),
  };
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kMacDataSum), checksum_buffer,
          sizeof(checksum_buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    if (entered_config_update) {
      ExitConfigUpdate(false);
    }
    return false;
  }
  DelayMs(10);

  if (entered_config_update && !ExitConfigUpdate(true)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ExitConfigUpdate failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ReadDataMemoryBytes(
    uint16_t address, uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > 32) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid argument\n");
    return false;
  }
  if (!Unseal()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Unseal failed\n");
    return false;
  }

  const uint8_t address_buffer[2] = {
      static_cast<uint8_t>(address),
      static_cast<uint8_t>(address >> 8),
  };
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kSelectSubclass), address_buffer,
          sizeof(address_buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kMacData), data, length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return true;
}

uint8_t Bq27220::CalcChecksum(const uint8_t* data, size_t length) {
  uint8_t sum = 0;
  for (size_t i = 0; i < length; ++i) {
    sum += data[i];
  }
  return 0xFF - sum;
}
}  // namespace cpp_bus_driver
