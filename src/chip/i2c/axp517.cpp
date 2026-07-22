/*
 * @Description: AXP517 电源管理芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-02-03 15:06:34
 * @LastEditTime: 2026-04-29 10:21:32
 * @License: GPL 3.0
 */
#include "axp517.h"

namespace cpp_bus_driver {
bool Axp517::Init(int32_t freq_hz) {
  if (rst_ != kDefaultValue) {
    bool result = true;
    result &= Tool::SetGpioMode(
        rst_, Tool::GpioMode::kOutput, Tool::GpioStatus::kPullup);
    result &= Tool::GpioWrite(rst_, 0);
    DelayMs(10);
    result &= Tool::GpioWrite(rst_, 1);
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

  auto buffer = GetDeviceId();
  if (buffer == static_cast<uint8_t>(-1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (buffer != kDefaultMessageHeaderInfo) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "AXP517 message header info is not default (value: %#X)\n", buffer);
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get axp517 message header info success (value: %#X)\n", buffer);
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  return true;
}

bool Axp517::Deinit(bool delete_bus) {
  bool result = true;

  if (!ChipI2cGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
    result = false;
  }

  if (rst_ != kDefaultValue) {
    result &= Tool::ResetGpio(rst_);
  }

  return result;
}

uint8_t Axp517::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwMessageHeaderInfo), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Axp517::GetChipStatus0(ChipStatus0& status) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoBmuStatus0), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  status.current_limit_status = buffer & 0B00000001;
  status.thermal_regulation_status = (buffer & 0B00000010) >> 1;
  status.battery_in_active_mode = (buffer & 0B00000100) >> 2;
  status.battery_present_status = (buffer & 0B00001000) >> 3;
  status.batfet_status = (buffer & 0B00010000) >> 4;
  status.vbus_good_indication = (buffer & 0B00100000) >> 5;

  return true;
}

bool Axp517::GetChipStatus1(ChipStatus1& status) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoBmuStatus1), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  status.charging_status = [](uint8_t status) -> ChargeStatus {
    switch (status) {
      case 0B00000000:
        return ChargeStatus::kTrickleCharge;
      case 0B00000001:
        return ChargeStatus::kPrecharge;
      case 0B00000010:
        return ChargeStatus::kConstantCurrent;
      case 0B00000011:
        return ChargeStatus::kConstantVoltage;
      case 0B00000100:
        return ChargeStatus::kChargeDone;
      case 0B00000101:
        return ChargeStatus::kNotCharging;
      default:
        return ChargeStatus::kInvalid;
    }
  }(buffer & 0B00000111);

  status.vindpm_status = (buffer & 0B00001000) >> 3;
  status.system_status_indication = (buffer & 0B00010000) >> 4;

  status.battery_current_direction =
      [](uint8_t status) -> BatteryCurrentDirection {
    switch (status) {
      case 0B00000000:
        return BatteryCurrentDirection::kStandby;
      case 0B00000001:
        return BatteryCurrentDirection::kCharge;
      case 0B00000010:
        return BatteryCurrentDirection::kDischarge;
      default:
        return BatteryCurrentDirection::kInvalid;
    }
  }((buffer & 0B01100000) >> 5);

  return true;
}

bool Axp517::GetIrqStatus(IrqStatus0& status0, IrqStatus1& status1,
    IrqStatus2& status2, IrqStatus3& status3) {
  uint8_t buffer[4] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwIrqStatus0), buffer,
          sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  status0.vbus_fault_flag = (buffer[0] & 0B00000001) >> 0;
  status0.vbus_over_voltage_flag = (buffer[0] & 0B00000010) >> 1;
  status0.boost_over_voltage_flag = (buffer[0] & 0B00000100) >> 2;
  status0.charge_to_normal_flag = (buffer[0] & 0B00001000) >> 3;
  status0.gauge_new_soc_flag = (buffer[0] & 0B00010000) >> 4;
  status0.soc_drop_to_shutdown_level_flag = (buffer[0] & 0B01000000) >> 6;
  status0.soc_drop_to_warning_level_flag = (buffer[0] & 0B10000000) >> 7;

  status1.pwr_on_positive_edge_flag = (buffer[1] & 0B00000001) >> 0;
  status1.pwr_on_negative_edge_flag = (buffer[1] & 0B00000010) >> 1;
  status1.pwr_on_long_press_flag = (buffer[1] & 0B00000100) >> 2;
  status1.pwr_on_short_press_flag = (buffer[1] & 0B00001000) >> 3;
  status1.battery_remove_flag = (buffer[1] & 0B00010000) >> 4;
  status1.battery_insert_flag = (buffer[1] & 0B00100000) >> 5;
  status1.vbus_remove_flag = (buffer[1] & 0B01000000) >> 6;
  status1.vbus_insert_flag = (buffer[1] & 0B10000000) >> 7;

  status2.battery_over_voltage_flag = (buffer[2] & 0B00000001) >> 0;
  status2.charger_safety_timer_expire_flag = (buffer[2] & 0B00000010) >> 1;
  status2.die_over_temperature_level1_flag =
      (buffer[2] & 0B00000100) >> 2;
  status2.charger_start_flag = (buffer[2] & 0B00001000) >> 3;
  status2.battery_charge_done_flag = (buffer[2] & 0B00010000) >> 4;
  status2.batfet_over_current_flag = (buffer[2] & 0B00100000) >> 5;
  status2.watchdog_expire_flag = (buffer[2] & 0B10000000) >> 7;

  status3.battery_under_temperature_work_flag =
      (buffer[3] & 0B00000001) >> 0;
  status3.battery_over_temperature_work_flag =
      (buffer[3] & 0B00000010) >> 1;
  status3.battery_under_temperature_charge_flag =
      (buffer[3] & 0B00000100) >> 2;
  status3.battery_over_temperature_charge_flag =
      (buffer[3] & 0B00001000) >> 3;
  status3.battery_over_temperature_quit_flag =
      (buffer[3] & 0B00010000) >> 4;
  status3.bc1_2_detect_result_change_flag =
      (buffer[3] & 0B01000000) >> 6;
  status3.bc1_2_detect_finished_flag = (buffer[3] & 0B10000000) >> 7;

  return true;
}

bool Axp517::ClearAllIrq() {
  const uint8_t clear_masks[] = {
      0B11011111,
      0B11111111,
      0B10111111,
      0B11011111,
  };

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwIrqStatus0), clear_masks,
          sizeof(clear_masks))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetChargeEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwModuleEnableControl1), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0B00000010;  // 设置bit1为1
  } else {
    buffer &= 0B11111101;  // 清除bit1
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwModuleEnableControl1), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetChargeCurrent(uint16_t current_ma) {
  // 范围检查：0-5120mA，64mA/步进
  if (current_ma > 5120) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    current_ma = 5120;
  }

  uint8_t buffer = current_ma / 64;

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwIccSetting), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetChargeVoltage(uint16_t voltage_mv) {
  uint8_t reg_value = 0;

  if (voltage_mv < 3700)  // 小于3.7V，选择3.6V
  {
    reg_value = 6;               // 3.6V
  } else if (voltage_mv < 3900)  // 3.7V-3.9V，选择3.8V
  {
    reg_value = 5;               // 3.8V
  } else if (voltage_mv < 4050)  // 3.9V-4.05V，选择4.0V
  {
    reg_value = 0;               // 4.0V
  } else if (voltage_mv < 4150)  // 4.05V-4.15V，选择4.1V
  {
    reg_value = 1;               // 4.1V
  } else if (voltage_mv < 4275)  // 4.15V-4.275V，选择4.2V
  {
    reg_value = 2;               // 4.2V
  } else if (voltage_mv < 4375)  // 4.275V-4.375V，选择4.35V
  {
    reg_value = 3;               // 4.35V
  } else if (voltage_mv < 4700)  // 4.375V-4.7V，选择4.4V
  {
    reg_value = 4;  // 4.4V
  } else            // 大于等于4.7V，选择5.0V
  {
    reg_value = 7;  // 5.0V
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwCvChargerVoltageSetting), reg_value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetInputCurrentLimit(uint16_t limit_ma) {
  // 范围检查：100-3250mA，50mA/步进
  if (limit_ma < 100) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    limit_ma = 100;
  } else if (limit_ma > 3250) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    limit_ma = 3250;
  }

  uint8_t buffer = ((limit_ma - 100) / 50) << 2;

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwInputCurrentLimitControl), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetInputVoltageLimit(uint16_t limit_mv) {
  // 范围检查：3600-16200mV，100mV/步进
  if (limit_mv < 3600) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    limit_mv = 3600;
  } else if (limit_mv > 16200) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    limit_mv = 16200;
  }

  uint8_t buffer = (limit_mv - 3600) / 100 + 1;

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwInputVoltageLimitControl), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint8_t Axp517::GetBatteryLevel() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoBatteryPercentage), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  return static_cast<int8_t>(buffer);
}

uint8_t Axp517::GetBatteryHealth() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoBatterySoh), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  return static_cast<int8_t>(buffer);
}

int8_t Axp517::GetBatteryTemperatureCelsius() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoBatteryTemperature), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  return static_cast<int8_t>(buffer);
}

bool Axp517::SetAdcChannel(AdcChannel channel) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwAdcChannelEnableControl),
          static_cast<uint8_t>(channel.vbus_current_measure << 7 |
                               channel.battery_discharge_current_measure << 6 |
                               channel.battery_charge_current_measure << 5 |
                               channel.chip_temperature_measure << 4 |
                               channel.system_voltage_measure << 3 |
                               channel.vbus_voltage_measure << 2 |
                               channel.ts_value_measure << 1 |
                               channel.battery_voltage_measure))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint16_t Axp517::GetBatteryVoltage() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoVbatH), buffer,
          sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[0] & 0B00111111) << 8) | buffer[1];
}

float Axp517::GetBatteryCurrent() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoIbatH), buffer,
          sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return static_cast<float>(static_cast<int16_t>(
             (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1])) /
         4.0;
}

float Axp517::GetTsVoltage() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoTsH), buffer,
          sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return static_cast<float>(
             (static_cast<uint16_t>(buffer[0] & 0B00111111) << 8) |
             buffer[1]) /
         2.0;
}

uint16_t Axp517::GetVbusCurrent() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoVbusCurrentH), buffer,
          sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[0] & 0B00111111) << 8) | buffer[1];
}

uint16_t Axp517::GetVbusVoltage() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoVbusVoltageH), buffer,
          sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return ((static_cast<uint16_t>(buffer[0] & 0B00111111) << 8) |
          buffer[1]) *
         2;
}

bool Axp517::SetAdcDataSelect(AdcData data_select) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwAdcDataSelect),
          static_cast<uint8_t>(data_select))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint16_t Axp517::GetAdcData() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoAdcDataH), buffer,
          sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
}

float Axp517::GetChipDieJunctionTemperatureCelsius() {
  uint16_t buffer = GetAdcData();

  if (buffer == static_cast<uint16_t>(-1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "GetAdcData failed\n");
    return -1;
  }

  buffer &= 0B0011111111111111;
  return static_cast<float>(3552 - buffer) / 1.79 + 25.0;
}

uint16_t Axp517::GetSystemVoltage() {
  uint16_t buffer = GetAdcData();

  if (buffer == static_cast<uint16_t>(-1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "GetAdcData failed\n");
    return -1;
  }

  return buffer & 0B0011111111111111;
}

float Axp517::GetChargingCurrent() {
  uint16_t buffer = GetAdcData();

  if (buffer == static_cast<uint16_t>(-1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "GetAdcData failed\n");
    return -1;
  }

  return static_cast<float>(buffer) / 4.0;
}

float Axp517::GetDischargingCurrent() {
  uint16_t buffer = GetAdcData();

  if (buffer == static_cast<uint16_t>(-1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "GetAdcData failed\n");
    return -1;
  }

  return static_cast<float>(static_cast<int32_t>(buffer) - 65536) / 4.0;
}

bool Axp517::SetBoostEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwModuleEnableControl1), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0B00010000;
  } else {
    buffer &= 0B11101111;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwModuleEnableControl1), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetGpioSource(GpioSource source) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwGpioConfigure), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B11110011) | (static_cast<uint8_t>(source) << 2);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwGpioConfigure), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetGpioMode(GpioMode mode) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwGpioConfigure), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (mode == GpioMode::kInput) {
    buffer &= 0B11101111;
  } else {
    buffer |= 0B00010000;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwGpioConfigure), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::GpioWrite(GpioStatus status) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwGpioConfigure), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B11111100) | static_cast<uint8_t>(status);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwGpioConfigure), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

Axp517::GpioStatus Axp517::GpioRead() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwGpioConfigure), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return GpioStatus::kInvalid;
  }

  if (((buffer & 0B00000010) >> 1) == 1) {
    return GpioStatus::kHigh;
  }

  return GpioStatus::kLow;
}

bool Axp517::SetShippingModeEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwBatfetControl), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0B00001000;
  } else {
    buffer &= 0B11110111;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwBatfetControl), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetForceBatfetMode(ForceBatfet mode) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwBatfetControl), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  switch (mode) {
    case ForceBatfet::kAuto:
      buffer &= 0B11111010;
      break;
    case ForceBatfet::kOn:
      buffer &= 0B11111011;
      buffer |= 0B00000001;
      break;
    case ForceBatfet::kOff:
      buffer &= 0B11111110;
      buffer |= 0B00000100;
      break;

    default:
      break;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwBatfetControl), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetForceRbfetEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwRbfetControl), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0B00000001;
  } else {
    buffer &= 0B11111110;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwRbfetControl), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetBoostVoltage(uint16_t voltage_mv) {
  if (voltage_mv < 4550) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    voltage_mv = 4550;
  } else if (voltage_mv > 5510) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    voltage_mv = 5510;
  }

  uint8_t value = static_cast<uint8_t>((voltage_mv - 4550) / 64);
  if (value > 15) {
    value = 15;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwBoostConfigure), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = static_cast<uint8_t>((buffer & 0B00001111) | (value << 4));

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwBoostConfigure), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetWatchdog(bool enable, uint8_t timeout_s) {
  uint8_t timeout_value = 0;
  if (timeout_s <= 1) {
    timeout_value = 0;
  } else if (timeout_s <= 2) {
    timeout_value = 1;
  } else if (timeout_s <= 4) {
    timeout_value = 2;
  } else if (timeout_s <= 8) {
    timeout_value = 3;
  } else if (timeout_s <= 16) {
    timeout_value = 4;
  } else if (timeout_s <= 32) {
    timeout_value = 5;
  } else if (timeout_s <= 64) {
    timeout_value = 6;
  } else {
    timeout_value = 7;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwWatchdogControl), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = static_cast<uint8_t>((buffer & 0B11111000) | timeout_value);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwWatchdogControl), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  const uint8_t enable_value = enable ? 0B00000001 : 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwModuleEnableControl0),
          &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = static_cast<uint8_t>((buffer & 0B11111110) | enable_value);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwModuleEnableControl0),
          buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwModuleEnableControl1),
          &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = static_cast<uint8_t>((buffer & 0B11111110) | enable_value);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwModuleEnableControl1),
          buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::FeedWatchdog() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwWatchdogControl), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer |= 0B00001000;

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwWatchdogControl), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetJeitaEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwJeitaStandardEnableControl),
          &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0B00000001;
  } else {
    buffer &= 0B11111110;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwJeitaStandardEnableControl),
          buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::SetBc12DetectEnable(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwModuleEnableControl0),
          &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0B00010000;
  } else {
    buffer &= 0B11101111;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwModuleEnableControl0),
          buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Axp517::GetBc12DetectResult(BcDetectResult& result) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoBcDetect), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  switch ((buffer & 0B11100000) >> 5) {
    case 1:
      result = BcDetectResult::kSdp;
      break;
    case 2:
      result = BcDetectResult::kCdp;
      break;
    case 3:
      result = BcDetectResult::kDcp;
      break;
    default:
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "Invalid BC detect result\n");
      return false;
  }

  return true;
}

bool Axp517::SetPdRole(bool is_source, bool is_drp) {
  const uint8_t cc_role = is_source ? 0B00000001 : 0B00000010;
  uint8_t value = static_cast<uint8_t>((cc_role << 2) | cc_role);
  if (is_drp) {
    value |= 0B01000000;
  }

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwRoleControl), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = static_cast<uint8_t>((buffer & 0B10110000) | value);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwRoleControl), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwMessageHeaderInfo), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = static_cast<uint8_t>(
      (buffer & 0B11110110) | (is_source ? 0B00001001 : 0));

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwMessageHeaderInfo), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (is_drp &&
      !bus_->Write(static_cast<uint8_t>(Cmd::kRwCommand),
          static_cast<uint8_t>(0x99))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
