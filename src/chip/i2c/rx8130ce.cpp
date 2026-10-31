/*
 * @Description: RX8130CE 实时时钟芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-10-31 11:00:00
 * @License: GPL 3.0
 */
#include "chip/i2c/rx8130ce.h"

#include <cstring>

namespace cpp_bus_driver {
namespace {
constexpr uint8_t kDaysPerMonth[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
}

uint8_t Rx8130ce::EncodeBcd(uint8_t value) {
  return static_cast<uint8_t>((value / 10U << 4U) | (value % 10U));
}

bool Rx8130ce::DecodeBcd(uint8_t value, uint8_t maximum, uint8_t& result) {
  if ((value & 0x0F) > 9 || (value >> 4) > 9) {
    return false;
  }
  const uint8_t decoded = static_cast<uint8_t>((value >> 4) * 10 +
      (value & 0x0F));
  if (decoded > maximum) {
    return false;
  }
  result = decoded;
  return true;
}

bool Rx8130ce::IsValidTime(const Time& time) {
  if (time.second > 59 || time.minute > 59 || time.hour > 23 ||
      static_cast<uint8_t>(time.weekday) > 6 || time.month < 1 ||
      time.month > 12 || time.day < 1 || time.year > 99) {
    return false;
  }
  uint8_t max_day = kDaysPerMonth[time.month - 1];
  if (time.month == 2 && time.year % 4 == 0) {
    ++max_day;
  }
  return time.day <= max_day;
}

uint8_t Rx8130ce::AlarmValue(uint8_t value, bool enabled) {
  return enabled ? EncodeBcd(value) : 0x80;
}

bool Rx8130ce::Init(int32_t freq_hz) {
  if (freq_hz <= 0 || freq_hz > 400000) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "RX8130CE invalid I2C frequency\n");
    return false;
  }
  if (!I2cChipBase::Init(freq_hz)) {
    return false;
  }
  // 官方要求至少设置一次 INIEN；默认保持 CHGEN=0，不启动充电。
  return SetPowerSwitchEnabled(true) && SetChargerEnabled(false);
}

bool Rx8130ce::Deinit(bool delete_bus) {
  return I2cChipBase::Deinit(delete_bus);
}

bool Rx8130ce::SoftwareReset() {
  return WriteRegisterRaw(static_cast<uint8_t>(Register::kControl0), 0x00) &&
         WriteRegisterRaw(static_cast<uint8_t>(Register::kControl0), 0x80) &&
         WriteRegisterRaw(0x50, 0x6C) && WriteRegisterRaw(0x53, 0x01) &&
         WriteRegisterRaw(0x66, 0x03) && WriteRegisterRaw(0x6B, 0x02) &&
         WriteRegisterRaw(0x6B, 0x01) && (DelayMs(125), true);
}

bool Rx8130ce::GetTime(Time& time) {
  for (int retry = 0; retry < 3; ++retry) {
    uint8_t data[7] = {};
    if (!ReadRegister(Register::kSecond, data, sizeof(data))) {
      return false;
    }
    Time result;
    const uint8_t weekday_data = data[3] & 0x7F;
    uint8_t weekday = 0;
    while (weekday < 7 && weekday_data != (1U << weekday)) {
      ++weekday;
    }
    if (DecodeBcd(data[0] & 0x7F, 59, result.second) &&
        DecodeBcd(data[1] & 0x7F, 59, result.minute) &&
        DecodeBcd(data[2] & 0x3F, 23, result.hour) &&
        (data[3] & 0x80) == 0 && weekday < 7 &&
        DecodeBcd(data[4] & 0x3F, 31, result.day) &&
        DecodeBcd(data[5] & 0x1F, 12, result.month) &&
        DecodeBcd(data[6], 99, result.year)) {
      result.weekday = static_cast<Weekday>(weekday);
      if (IsValidTime(result)) {
        time = result;
        return true;
      }
    }
    DelayMs(2);
    if (retry == 2) {
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "RX8130CE invalid calendar data: %02X %02X %02X %02X %02X %02X %02X\n",
          data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
    }
  }
  return false;
}

bool Rx8130ce::SetTime(const Time& time) {
  if (!IsValidTime(time)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "RX8130CE invalid time argument\n");
    return false;
  }
  const uint8_t data[] = {
      EncodeBcd(time.second), EncodeBcd(time.minute), EncodeBcd(time.hour),
      static_cast<uint8_t>(1U << static_cast<uint8_t>(time.weekday)),
      EncodeBcd(time.day),
      EncodeBcd(time.month), EncodeBcd(time.year)};
  return UpdateControl0(0x40, 0x40) &&
         WriteRegister(Register::kSecond, data, sizeof(data)) &&
         UpdateControl0(0x40, 0);
}

bool Rx8130ce::GetStatus(Status& status) {
  uint8_t flag = 0;
  uint8_t control0 = 0;
  uint8_t control1 = 0;
  uint8_t extension = 0;
  if (!ReadRegister(Register::kFlag, flag) ||
      !ReadRegister(Register::kControl0, control0) ||
      !ReadRegister(Register::kControl1, control1) ||
      !ReadRegister(Register::kExtension, extension)) {
    return false;
  }
  Status result;
  result.battery_low = (flag & 0x80) != 0;
  result.update_flag = (flag & 0x20) != 0;
  result.timer_flag = (flag & 0x10) != 0;
  result.alarm_flag = (flag & 0x08) != 0;
  result.reset_flag = (flag & 0x04) != 0;
  result.voltage_loss = (flag & 0x02) != 0;
  result.battery_full = (flag & 0x01) != 0;
  result.charger_enabled = (control1 & 0x20) != 0;
  result.power_switch_enabled = (control1 & 0x10) != 0;
  result.update_interrupt_enabled = (control0 & 0x20) != 0;
  result.timer_interrupt_enabled = (control0 & 0x10) != 0;
  result.alarm_interrupt_enabled = (control0 & 0x08) != 0;
  result.timer_enabled = (extension & 0x10) != 0;
  result.clock_stopped = (control0 & 0x40) != 0;
  result.clock_output = static_cast<ClockOutput>(extension >> 6);
  status = result;
  return true;
}

bool Rx8130ce::ClearVoltageLossFlag() { return UpdateFlag(0x02); }
bool Rx8130ce::ClearBatteryLowFlag() { return UpdateFlag(0x80); }
bool Rx8130ce::ClearResetFlag() { return UpdateFlag(0x04); }

bool Rx8130ce::SetPowerSwitchEnabled(bool enabled) {
  return UpdateControl1(0x10,
      enabled ? 0x10 : 0);
}

bool Rx8130ce::GetPowerSwitchEnabled(bool& enabled) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kControl1, value)) return false;
  enabled = (value & 0x10) != 0;
  return true;
}

bool Rx8130ce::SetChargerEnabled(bool enabled) {
  return UpdateControl1(0x20,
      enabled ? 0x20 : 0);
}

bool Rx8130ce::GetChargerEnabled(bool& enabled) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kControl1, value)) return false;
  enabled = (value & 0x20) != 0;
  return true;
}

bool Rx8130ce::SetResetVoltage(ResetVoltage voltage) {
  const uint8_t value = static_cast<uint8_t>(voltage);
  if (value > 1) return false;
  return UpdateControl1(0x04,
      value ? 0x04 : 0);
}

bool Rx8130ce::GetResetVoltage(ResetVoltage& voltage) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kControl1, value)) return false;
  voltage = static_cast<ResetVoltage>((value & 0x04) != 0);
  return true;
}

bool Rx8130ce::SetBatteryFullVoltage(BatteryFullVoltage voltage) {
  const uint8_t value = static_cast<uint8_t>(voltage);
  if (value > 3) return false;
  return UpdateControl1(0x03, value);
}

bool Rx8130ce::GetBatteryFullVoltage(BatteryFullVoltage& voltage) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kControl1, value)) return false;
  voltage = static_cast<BatteryFullVoltage>(
      value & 0x03);
  return true;
}

bool Rx8130ce::SetBatteryLowDetectionEnabled(bool enabled) {
  return UpdateExtension1(0x01,
      enabled ? 0x01 : 0);
}

bool Rx8130ce::GetBatteryLowDetectionEnabled(bool& enabled) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kExtension1, value))
    return false;
  enabled = (value & 0x01) != 0;
  return true;
}

bool Rx8130ce::SetClockOutput(ClockOutput output) {
  const uint8_t value = static_cast<uint8_t>(output);
  if (value > 3) return false;
  uint8_t extension = 0;
  if (!ReadRegister(Register::kExtension, extension)) return false;
  extension = static_cast<uint8_t>((extension & 0x3F) | (value << 6));
  return WriteRegister(Register::kExtension, extension);
}

bool Rx8130ce::GetClockOutput(ClockOutput& output) {
  uint8_t extension = 0;
  if (!ReadRegister(Register::kExtension, extension)) return false;
  output = static_cast<ClockOutput>(extension >> 6);
  return true;
}

bool Rx8130ce::SetAlarm(const Alarm& alarm) {
  if ((alarm.minute_enabled && alarm.minute > 59) ||
      (alarm.hour_enabled && alarm.hour > 23) ||
      (alarm.day_or_weekday_enabled &&
          (alarm.weekday_mode ? alarm.day_or_weekday > 6
                              : alarm.day_or_weekday < 1 ||
                                    alarm.day_or_weekday > 31))) {
    return false;
  }
  const uint8_t data[] = {
      AlarmValue(alarm.minute, alarm.minute_enabled),
      AlarmValue(alarm.hour, alarm.hour_enabled),
      alarm.day_or_weekday_enabled
          ? static_cast<uint8_t>(alarm.weekday_mode
                                     ? alarm.day_or_weekday
                                     : EncodeBcd(alarm.day_or_weekday))
          : uint8_t{0x80}};
  uint8_t control = 0;
  if (!ReadRegister(Register::kControl0, control) ||
      !UpdateControl0(0x08, 0) ||
      !WriteRegister(Register::kAlarmMinute, data, sizeof(data))) return false;
  return UpdateExtension(0x08,
             alarm.weekday_mode ? 0x08 : 0) &&
      UpdateControl0(0x08,
          control & 0x08);
}

bool Rx8130ce::GetAlarm(Alarm& alarm) {
  uint8_t data[3] = {};
  if (!ReadRegister(Register::kAlarmMinute, data, sizeof(data))) return false;
  uint8_t extension = 0;
  if (!ReadRegister(Register::kExtension, extension)) return false;
  Alarm result;
  result.minute_enabled = (data[0] & 0x80) == 0;
  result.hour_enabled = (data[1] & 0x80) == 0;
  result.day_or_weekday_enabled = (data[2] & 0x80) == 0;
  result.weekday_mode = (extension & 0x08) != 0;
  if ((result.minute_enabled && !DecodeBcd(data[0], 59, result.minute)) ||
      (result.hour_enabled && !DecodeBcd(data[1], 23, result.hour)) ||
      (result.day_or_weekday_enabled &&
          (result.weekday_mode ? data[2] > 6
                               : !DecodeBcd(data[2], 31,
                                     result.day_or_weekday)))) return false;
  if (result.weekday_mode && result.day_or_weekday_enabled)
    result.day_or_weekday = data[2];
  alarm = result;
  return true;
}

bool Rx8130ce::SetAlarmInterruptEnabled(bool enabled) {
  return UpdateControl0(0x08,
      enabled ? 0x08 : 0);
}
bool Rx8130ce::ClearAlarmFlag() { return UpdateFlag(0x08); }

bool Rx8130ce::SetTimer(const Timer& timer) {
  const uint8_t clock = static_cast<uint8_t>(timer.clock);
  if (clock > 4 ||
      !UpdateExtension(0x10, 0) ||
      !WriteRegister(Register::kTimerCounter0, static_cast<uint8_t>(timer.counter)) ||
      !WriteRegister(Register::kTimerCounter1,
          static_cast<uint8_t>(timer.counter >> 8)) ||
      !UpdateExtension(0x07, clock)) return false;
  return !timer.enabled || UpdateExtension(0x10,
      0x10);
}

bool Rx8130ce::GetTimer(Timer& timer) {
  uint8_t data[2] = {};
  if (!ReadRegister(Register::kTimerCounter0, data, sizeof(data))) return false;
  uint8_t extension = 0;
  if (!ReadRegister(Register::kExtension, extension)) return false;
  timer.counter = static_cast<uint16_t>(data[0] | data[1] << 8);
  timer.clock = static_cast<TimerClock>(extension & 0x07);
  timer.enabled = (extension & 0x10) != 0;
  return static_cast<uint8_t>(timer.clock) <= 4;
}

bool Rx8130ce::StopTimer() { return UpdateExtension(0x10, 0); }
bool Rx8130ce::SetTimerInterruptEnabled(bool enabled) {
  return UpdateControl0(0x10,
      enabled ? 0x10 : 0);
}
bool Rx8130ce::ClearTimerFlag() { return UpdateFlag(0x10); }

bool Rx8130ce::SetUpdateInterruptEnabled(bool enabled) {
  return UpdateControl0(0x20,
      enabled ? 0x20 : 0);
}

bool Rx8130ce::ClearUpdateFlag() { return UpdateFlag(0x20); }

bool Rx8130ce::GetUserRam(uint8_t* data, size_t length) {
  return ReadRegister(Register::kUserRam0, data, length);
}

bool Rx8130ce::SetUserRam(const uint8_t* data, size_t length) {
  return WriteRegister(Register::kUserRam0, data, length);
}

bool Rx8130ce::GetDigitalOffset(uint8_t& value) {
  return ReadRegister(Register::kDigitalOffset, value);
}

bool Rx8130ce::SetDigitalOffset(uint8_t value) {
  return WriteRegister(Register::kDigitalOffset, value);
}

bool Rx8130ce::SetExtension1(uint8_t value) {
  if (value & static_cast<uint8_t>(~0x01)) return false;
  return WriteRegister(Register::kExtension1, value);
}

bool Rx8130ce::GetExtension1(uint8_t& value) {
  return ReadRegister(Register::kExtension1, value);
}

bool Rx8130ce::ReadRegister(Register address, uint8_t* data, size_t length) {
  return ReadRegister(static_cast<uint8_t>(address), data, length);
}

bool Rx8130ce::ReadRegister(Register address, uint8_t& value) {
  return ReadRegister(static_cast<uint8_t>(address), value);
}

bool Rx8130ce::WriteRegister(Register address, const uint8_t* data,
    size_t length) {
  return WriteRegister(static_cast<uint8_t>(address), data, length);
}

bool Rx8130ce::WriteRegister(Register address, uint8_t value) {
  return WriteRegister(static_cast<uint8_t>(address), value);
}

bool Rx8130ce::UpdateControl0(uint8_t mask, uint8_t value) {
  uint8_t data = 0;
  return ReadRegister(Register::kControl0, data) &&
      WriteRegister(Register::kControl0, static_cast<uint8_t>((data & ~mask) |
          (value & mask)));
}

bool Rx8130ce::UpdateControl1(uint8_t mask, uint8_t value) {
  uint8_t data = 0;
  return ReadRegister(Register::kControl1, data) &&
      WriteRegister(Register::kControl1, static_cast<uint8_t>((data & ~mask) |
          (value & mask)));
}

bool Rx8130ce::UpdateExtension(uint8_t mask, uint8_t value) {
  uint8_t data = 0;
  return ReadRegister(Register::kExtension, data) &&
      WriteRegister(Register::kExtension, static_cast<uint8_t>((data & ~mask) |
          (value & mask)));
}

bool Rx8130ce::UpdateFlag(uint8_t mask) {
  // 标志位采用写 0 清除、写 1 保持，不能直接写回读取值。
  uint8_t data = 0;
  if (!ReadRegister(Register::kFlag, data)) return false;
  return WriteRegister(Register::kFlag, static_cast<uint8_t>(data & ~mask));
}

bool Rx8130ce::UpdateExtension1(uint8_t mask, uint8_t value) {
  uint8_t data = 0;
  if (!ReadRegister(Register::kExtension1, data))
    return false;
  data = static_cast<uint8_t>((data & ~mask) | (value & mask));
  return WriteRegister(Register::kExtension1, data);
}

bool Rx8130ce::IsUserRegisterRange(uint8_t address, size_t length) {
  if (length == 0) return false;
  const size_t end = static_cast<size_t>(address) + length - 1;
  const bool main_registers = address >= 0x10 && end <= 0x23;
  const bool offset_register = address == 0x30 && length == 1;
  const bool extension_register = address == 0x31 && length == 1;
  return main_registers || offset_register || extension_register;
}

bool Rx8130ce::ReadRegister(uint8_t address, uint8_t& value) {
  return ReadRegister(address, &value, sizeof(value));
}

bool Rx8130ce::WriteRegister(uint8_t address, uint8_t value) {
  return WriteRegister(address, &value, sizeof(value));
}

bool Rx8130ce::ReadRegisterRaw(uint8_t address, uint8_t& value) {
  return bus_ != nullptr && bus_->WriteRead(&address, 1, &value, 1);
}

bool Rx8130ce::WriteRegisterRaw(uint8_t address, uint8_t value) {
  const uint8_t packet[] = {address, value};
  return bus_ != nullptr && bus_->Write(packet, sizeof(packet));
}

bool Rx8130ce::ReadRegister(uint8_t address, uint8_t* data, size_t length) {
  if (!IsUserRegisterRange(address, length) || data == nullptr ||
      bus_ == nullptr || !bus_->WriteRead(&address, 1, data, length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "RX8130CE register read failed (register: %#X, size: %zu)\n",
        address, length);
    return false;
  }
  return true;
}

bool Rx8130ce::WriteRegister(uint8_t address, const uint8_t* data,
    size_t length) {
  if (!IsUserRegisterRange(address, length) || data == nullptr ||
      bus_ == nullptr || length > 20) return false;
  uint8_t packet[1 + 20] = {};
  packet[0] = address;
  std::memcpy(packet + 1, data, length);
  return bus_->Write(packet, length + 1);
}

}  // namespace cpp_bus_driver
