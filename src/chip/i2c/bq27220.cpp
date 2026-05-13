/*
 * @Description: BQ27220 single-cell CEDV fuel gauge driver
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-05-12 18:57:34
 * @License: GPL 3.0
 */
#include "bq27220.h"

namespace cpp_bus_driver {
bool Bq27220::Init(int32_t freq_hz) {
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

  const uint16_t device_id = GetDeviceId();
  if (device_id != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get bq27220 device id failed (error id: %#X)\n", device_id);
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "Get bq27220 device id success (id: %#X)\n", device_id);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "bq27220 fw: %#X, hw: %#X\n",
      GetFirmwareVersion(), GetHardwareVersion());
  return true;
}

bool Bq27220::Deinit(bool delete_bus) {
  if (!ChipI2cGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kDisable, GpioStatus::kDisable);
  }

  return true;
}

uint16_t Bq27220::GetDeviceId() {
  uint16_t value = 0;
  ReadControlSubcommand(ControlSubcommand::kDeviceNumber, &value);
  return value;
}

uint16_t Bq27220::GetFirmwareVersion() {
  uint16_t value = 0;
  ReadControlSubcommand(ControlSubcommand::kFirmwareVersion, &value);
  return value;
}

uint16_t Bq27220::GetHardwareVersion() {
  uint16_t value = 0;
  ReadControlSubcommand(ControlSubcommand::kHardwareVersion, &value);
  return value;
}

uint16_t Bq27220::GetDesignCapacity() {
  uint16_t value = 0;
  ReadU16(Cmd::kDesignCapacity, &value);
  return value;
}

uint16_t Bq27220::GetVoltage() {
  uint16_t value = 0;
  ReadU16(Cmd::kVoltage, &value);
  return value;
}

int16_t Bq27220::GetCurrent() {
  int16_t value = 0;
  ReadS16(Cmd::kCurrent, &value);
  return value;
}

int16_t Bq27220::GetAverageCurrent() {
  int16_t value = 0;
  ReadS16(Cmd::kAverageCurrent, &value);
  return value;
}

uint16_t Bq27220::GetRemainingCapacity() {
  uint16_t value = 0;
  ReadU16(Cmd::kRemainingCapacity, &value);
  return value;
}

uint16_t Bq27220::GetFullChargeCapacity() {
  uint16_t value = 0;
  ReadU16(Cmd::kFullChargeCapacity, &value);
  return value;
}

int16_t Bq27220::GetAtRate() {
  int16_t value = 0;
  ReadS16(Cmd::kAtRate, &value);
  return value;
}

bool Bq27220::SetAtRate(int16_t rate) {
  return WriteU16(Cmd::kAtRate, static_cast<uint16_t>(rate));
}

uint16_t Bq27220::GetAtRateTimeToEmpty() {
  uint16_t value = 0;
  ReadU16(Cmd::kAtRateTimeToEmpty, &value);
  return value;
}

uint16_t Bq27220::GetTemperatureRaw() {
  uint16_t value = 0;
  ReadU16(Cmd::kTemperature, &value);
  return value;
}

float Bq27220::GetTemperatureKelvin() { return GetTemperatureRaw() * 0.1f; }

float Bq27220::GetTemperatureCelsius() {
  return GetTemperatureKelvin() - 273.15f;
}

bool Bq27220::SetTemperatureMode(TemperatureMode mode) {
  if (!Unseal()) {
    return false;
  }

  uint16_t operation_config = 0;
  if (!ReadDataMemory(
          DataMemoryAddress::kOperationConfigA, &operation_config)) {
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

  return WriteDataMemory(
      DataMemoryAddress::kOperationConfigA, operation_config);
}

bool Bq27220::GetBatteryStatus(BatteryStatus& status) {
  uint16_t value = 0;
  if (!ReadU16(Cmd::kBatteryStatus, &value)) {
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
  return WriteDataMemory(DataMemoryAddress::kDesignCapacity, capacity);
}

uint16_t Bq27220::GetTimeToEmpty() {
  uint16_t value = 0;
  ReadU16(Cmd::kTimeToEmpty, &value);
  return value;
}

uint16_t Bq27220::GetTimeToFull() {
  uint16_t value = 0;
  ReadU16(Cmd::kTimeToFull, &value);
  return value;
}

int16_t Bq27220::GetStandbyCurrent() {
  int16_t value = 0;
  ReadS16(Cmd::kStandbyCurrent, &value);
  return value;
}

uint16_t Bq27220::GetStandbyTimeToEmpty() {
  uint16_t value = 0;
  ReadU16(Cmd::kStandbyTimeToEmpty, &value);
  return value;
}

int16_t Bq27220::GetMaxLoadCurrent() {
  int16_t value = 0;
  ReadS16(Cmd::kMaxLoadCurrent, &value);
  return value;
}

uint16_t Bq27220::GetMaxLoadTimeToEmpty() {
  uint16_t value = 0;
  ReadU16(Cmd::kMaxLoadTimeToEmpty, &value);
  return value;
}

int16_t Bq27220::GetRawCoulombCount() {
  int16_t value = 0;
  ReadS16(Cmd::kRawCoulombCount, &value);
  return value;
}

int16_t Bq27220::GetAveragePower() {
  int16_t value = 0;
  ReadS16(Cmd::kAveragePower, &value);
  return value;
}

uint16_t Bq27220::GetChipTemperatureRaw() {
  uint16_t value = 0;
  ReadU16(Cmd::kInternalTemperature, &value);
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
  ReadU16(Cmd::kCycleCount, &value);
  return value;
}

uint16_t Bq27220::GetStatusOfCharge() {
  uint16_t value = 0;
  ReadU16(Cmd::kStatusOfCharge, &value);
  return value;
}

uint16_t Bq27220::GetStatusOfHealth() {
  uint16_t value = 0;
  ReadU16(Cmd::kStatusOfHealth, &value);
  return value;
}

uint16_t Bq27220::GetChargingVoltage() {
  uint16_t value = 0;
  ReadU16(Cmd::kChargingVoltage, &value);
  return value;
}

uint16_t Bq27220::GetChargingCurrent() {
  uint16_t value = 0;
  ReadU16(Cmd::kChargingCurrent, &value);
  return value;
}

bool Bq27220::SetBtpDischargeThreshold(uint16_t threshold_mah) {
  return WriteU16(Cmd::kBtpDischargeSet, threshold_mah);
}

bool Bq27220::SetBtpChargeThreshold(uint16_t threshold_mah) {
  return WriteU16(Cmd::kBtpChargeSet, threshold_mah);
}

uint16_t Bq27220::GetAnalogCount() {
  uint16_t value = 0;
  ReadU16(Cmd::kAnalogCount, &value);
  return value;
}

int16_t Bq27220::GetRawCurrent() {
  int16_t value = 0;
  ReadS16(Cmd::kRawCurrent, &value);
  return value;
}

uint16_t Bq27220::GetRawVoltage() {
  uint16_t value = 0;
  ReadU16(Cmd::kRawVoltage, &value);
  return value;
}

uint16_t Bq27220::GetRawInternalTemperature() {
  uint16_t value = 0;
  ReadU16(Cmd::kRawInternalTemperature, &value);
  return value;
}

bool Bq27220::SetSleepCurrentThreshold(uint16_t threshold) {
  if (threshold > 100) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Sleep current threshold out of range, clamp to 100mA\n");
    threshold = 100;
  }

  return WriteDataMemory(DataMemoryAddress::kSleepCurrent, threshold);
}

bool Bq27220::SendControlSubcommand(ControlSubcommand subcommand) {
  return WriteU16(Cmd::kControl, static_cast<uint16_t>(subcommand));
}

bool Bq27220::ReadControlSubcommand(
    ControlSubcommand subcommand, uint16_t* value) {
  if (value == nullptr) {
    return false;
  }
  if (!SendControlSubcommand(subcommand)) {
    return false;
  }
  DelayMs(15);
  return ReadU16(Cmd::kMacData, value);
}

bool Bq27220::Seal() {
  if (!SendControlSubcommand(ControlSubcommand::kSeal)) {
    return false;
  }
  DelayMs(10);

  OperationStatus status;
  return GetOperationStatus(status) && status.security == SecurityMode::kSealed;
}

bool Bq27220::Unseal() {
  OperationStatus status;
  if (GetOperationStatus(status) &&
      (status.security == SecurityMode::kUnsealed ||
          status.security == SecurityMode::kFullAccess)) {
    return true;
  }

  if (!WriteU16(Cmd::kControl, kUnsealKey1)) {
    return false;
  }
  DelayMs(10);
  if (!WriteU16(Cmd::kControl, kUnsealKey2)) {
    return false;
  }
  DelayMs(10);

  return GetOperationStatus(status) &&
         (status.security == SecurityMode::kUnsealed ||
             status.security == SecurityMode::kFullAccess);
}

bool Bq27220::FullAccess() {
  OperationStatus status;
  if (GetOperationStatus(status) &&
      status.security == SecurityMode::kFullAccess) {
    return true;
  }

  if (!Unseal()) {
    return false;
  }
  if (!WriteU16(Cmd::kControl, kFullAccessKey)) {
    return false;
  }
  DelayMs(10);
  if (!WriteU16(Cmd::kControl, kFullAccessKey)) {
    return false;
  }
  DelayMs(10);

  return GetOperationStatus(status) &&
         status.security == SecurityMode::kFullAccess;
}

bool Bq27220::Reset() {
  if (!SendControlSubcommand(ControlSubcommand::kReset)) {
    return false;
  }
  DelayMs(100);
  return GetDeviceId() == kDeviceId;
}

bool Bq27220::SetBatteryInserted(bool inserted) {
  return SendControlSubcommand(inserted ? ControlSubcommand::kBatteryInsert
                                        : ControlSubcommand::kBatteryRemove);
}

bool Bq27220::SetBatteryProfile(uint8_t profile) {
  if (profile < 1 || profile > 6) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Battery profile index out of range\n");
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
  return SendControlSubcommand(subcommands[profile - 1]);
}

bool Bq27220::EnterCalibration() {
  return SendControlSubcommand(ControlSubcommand::kEnterCalibration);
}

bool Bq27220::ExitCalibration() {
  return SendControlSubcommand(ControlSubcommand::kExitCalibration);
}

bool Bq27220::ToggleCalibration() {
  return SendControlSubcommand(ControlSubcommand::kCalibrationToggle);
}

bool Bq27220::ReadDataMemory(DataMemoryAddress address, uint16_t* value) {
  return ReadDataMemory(static_cast<uint16_t>(address), value);
}

bool Bq27220::ReadDataMemory(DataMemoryAddress address, uint8_t* value) {
  return ReadDataMemory(static_cast<uint16_t>(address), value);
}

bool Bq27220::ReadDataMemory(uint16_t address, uint16_t* value) {
  if (value == nullptr) {
    return false;
  }

  uint8_t data[2] = {};
  if (!ReadDataMemoryBytes(address, data, 2)) {
    return false;
  }
  *value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  return true;
}

bool Bq27220::ReadDataMemory(uint16_t address, uint8_t* value) {
  if (value == nullptr) {
    return false;
  }
  return ReadDataMemoryBytes(address, value, 1);
}

bool Bq27220::WriteDataMemory(DataMemoryAddress address, uint16_t value) {
  return WriteDataMemory(static_cast<uint16_t>(address), value);
}

bool Bq27220::WriteDataMemory(DataMemoryAddress address, uint8_t value) {
  return WriteDataMemory(static_cast<uint16_t>(address), value);
}

bool Bq27220::WriteDataMemory(uint16_t address, uint16_t value) {
  const uint8_t data[2] = {
      static_cast<uint8_t>(value >> 8),
      static_cast<uint8_t>(value),
  };
  return WriteDataMemoryBytes(address, data, 2);
}

bool Bq27220::WriteDataMemory(uint16_t address, uint8_t value) {
  return WriteDataMemoryBytes(address, &value, 1);
}

bool Bq27220::ApplyBatteryProfile(
    const CedvProfile& profile, const GaugingConfig& config) {
  if (!EnterConfigUpdate()) {
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
        LogLevel::kChip, __FILE__, __LINE__, "ApplyBatteryProfile failed\n");
  }
  return result;
}

bool Bq27220::ApplyBatteryProfileIfNeeded(
    const CedvProfile& profile, const GaugingConfig& config) {
  if (!Unseal()) {
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

  return ApplyBatteryProfile(profile, config);
}

bool Bq27220::EnterConfigUpdate() {
  if (!Unseal()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Unseal failed\n");
    return false;
  }

  if (!SendControlSubcommand(ControlSubcommand::kEnterConfigUpdate)) {
    return false;
  }
  DelayMs(10);
  return WaitConfigUpdate(true);
}

bool Bq27220::ExitConfigUpdate(bool reinit) {
  if (!SendControlSubcommand(reinit ? ControlSubcommand::kExitConfigUpdateReinit
                                    : ControlSubcommand::kExitConfigUpdate)) {
    return false;
  }
  DelayMs(10);
  return WaitConfigUpdate(false);
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

  LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WaitConfigUpdate timeout\n");
  return false;
}

bool Bq27220::ReadU16(Cmd cmd, uint16_t* value) {
  if (value == nullptr) {
    return false;
  }

  uint8_t buffer[2] = {};
  if (!bus_->Read(static_cast<uint8_t>(cmd), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  *value = (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
  return true;
}

bool Bq27220::WriteU16(Cmd cmd, uint16_t value) {
  if (!bus_->Write(static_cast<uint8_t>(cmd), value, Endian::kLittle)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  return true;
}

bool Bq27220::ReadS16(Cmd cmd, int16_t* value) {
  uint16_t raw = 0;
  if (!ReadU16(cmd, &raw)) {
    return false;
  }
  *value = static_cast<int16_t>(raw);
  return true;
}

bool Bq27220::WriteDataMemoryBytes(
    uint16_t address, const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > 32) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Invalid data memory write length\n");
    return false;
  }

  bool entered_config_update = false;
  OperationStatus status;
  if (!GetOperationStatus(status)) {
    return false;
  }
  if (!status.flag.config_update_mode) {
    if (!EnterConfigUpdate()) {
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
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
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
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    if (entered_config_update) {
      ExitConfigUpdate(false);
    }
    return false;
  }
  DelayMs(10);

  if (entered_config_update && !ExitConfigUpdate(true)) {
    return false;
  }
  return true;
}

bool Bq27220::ReadDataMemoryBytes(
    uint16_t address, uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > 32) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Invalid data memory read length\n");
    return false;
  }
  if (!Unseal()) {
    return false;
  }

  const uint8_t address_buffer[2] = {
      static_cast<uint8_t>(address),
      static_cast<uint8_t>(address >> 8),
  };
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kSelectSubclass), address_buffer,
          sizeof(address_buffer))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kMacData), data, length)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
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
