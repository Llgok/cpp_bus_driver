/*
 * @Description: PCF8563 系列实时时钟芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:51
 * @LastEditTime: 2026-09-10 14:51:34
 * @License: GPL 3.0
 */
#include "chip/i2c/pcf8563x.h"

#include <cstring>

namespace cpp_bus_driver {
namespace {

/**
 * @brief 将两位十进制数编码为压缩 BCD
 * @param value 十进制数，调用方应保证范围为 0~99
 * @return 高半字节为十位，低半字节为个位的 BCD 编码
 */
uint8_t EncodeBcd(uint8_t value) {
  return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

/**
 * @brief 校验并解码压缩 BCD
 * @param value 已去除状态位的 BCD 编码
 * @param minimum 允许的最小十进制值
 * @param maximum 允许的最大十进制值
 * @param decoded 接收十进制值，仅成功时更新
 * @return BCD 合法且数值在范围内返回 true，否则返回 false
 */
bool DecodeBcd(uint8_t value, uint8_t minimum, uint8_t maximum,
    uint8_t& decoded) {
  if ((value & 0x0F) > 9 || (value >> 4) > 9) {
    return false;
  }
  const uint8_t result = (value >> 4) * 10 + (value & 0x0F);
  if (result < minimum || result > maximum) {
    return false;
  }
  decoded = result;
  return true;
}

/**
 * @brief 按芯片的四年闰年规则校验日期和时间
 * @param time 待检查的日期时间，不校验星期与日期的一致性
 * @return 所有字段及月份天数合法返回 true，否则返回 false
 */
bool IsValidTime(const Pcf8563x::Time& time) {
  if (time.second > 59 || time.minute > 59 || time.hour > 23 ||
      static_cast<uint8_t>(time.week) > 6 || time.month < 1 ||
      time.month > 12 || time.year > 99 || time.day < 1) {
    return false;
  }
  constexpr uint8_t kDaysPerMonth[] = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint8_t maximum = kDaysPerMonth[time.month - 1];
  if (time.month == 2 && time.year % 4 == 0) {
    ++maximum;
  }
  return time.day <= maximum;
}

}  // namespace

bool Pcf8563x::Init(int32_t freq_hz) {
  if (freq_hz <= 0 || freq_hz > 400000) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "PCF8563 invalid I2C frequency\n");
    return false;
  }
  if (!I2cChipBase::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Init failed\n");
    return false;
  }
  uint8_t control = 0;
  // 正常运行要求 TEST1/TESTC 和 N 位为零，保留原有 STOP 状态。
  return ReadRegister(Register::kControlStatus1, &control) &&
         WriteRegister(Register::kControlStatus1,
             static_cast<uint8_t>(control & 0x20));
}

bool Pcf8563x::Deinit(bool delete_bus) {
  return I2cChipBase::Deinit(delete_bus);
}

bool Pcf8563x::GetStatus(Status& status) {
  uint8_t data[3] = {};
  if (!ReadRegister(Register::kControlStatus1, data, sizeof(data))) {
    return false;
  }
  Status result;
  result.clock_stopped = (data[0] & 0x20) != 0;
  result.voltage_low = (data[2] & 0x80) != 0;
  result.alarm_flag = (data[1] & 0x08) != 0;
  result.timer_flag = (data[1] & 0x04) != 0;
  result.alarm_interrupt_enabled = (data[1] & 0x02) != 0;
  result.timer_interrupt_enabled = (data[1] & 0x01) != 0;
  result.timer_interrupt_mode = (data[1] & 0x10) != 0
                                    ? TimerInterruptMode::kPulse
                                    : TimerInterruptMode::kLevel;
  status = result;
  return true;
}

bool Pcf8563x::SetClockEnabled(bool enabled) {
  return WriteRegister(Register::kControlStatus1, enabled ? 0x00 : 0x20);
}

bool Pcf8563x::GetTime(Time& time, bool& voltage_low) {
  uint8_t data[7] = {};
  if (!ReadRegister(Register::kVlSeconds, data, sizeof(data))) {
    return false;
  }
  Time result;
  if (!DecodeBcd(data[0] & 0x7F, 0, 59, result.second) ||
      !DecodeBcd(data[1] & 0x7F, 0, 59, result.minute) ||
      !DecodeBcd(data[2] & 0x3F, 0, 23, result.hour) ||
      !DecodeBcd(data[3] & 0x3F, 1, 31, result.day) ||
      (data[4] & 0x07) > 6 ||
      !DecodeBcd(data[5] & 0x1F, 1, 12, result.month) ||
      !DecodeBcd(data[6], 0, 99, result.year)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "PCF8563 invalid calendar register data\n");
    return false;
  }
  result.week = static_cast<Week>(data[4] & 0x07);
  result.century = (data[5] & 0x80) != 0;
  if (!IsValidTime(result)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "PCF8563 invalid calendar date\n");
    return false;
  }
  time = result;
  voltage_low = (data[0] & 0x80) != 0;
  return true;
}

bool Pcf8563x::SetTime(const Time& time) {
  if (!IsValidTime(time)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "PCF8563 invalid time argument\n");
    return false;
  }
  const uint8_t data[] = {
      EncodeBcd(time.second), EncodeBcd(time.minute), EncodeBcd(time.hour),
      EncodeBcd(time.day), static_cast<uint8_t>(time.week),
      static_cast<uint8_t>(EncodeBcd(time.month) | (time.century ? 0x80 : 0)),
      EncodeBcd(time.year)};
  return WriteRegister(Register::kVlSeconds, data, sizeof(data));
}

bool Pcf8563x::GetClockOut(ClockOutConfig& config) {
  uint8_t value = 0;
  if (!ReadRegister(Register::kClkoutControl, &value)) {
    return false;
  }
  config.enabled = (value & 0x80) != 0;
  config.frequency = static_cast<ClockOutFrequency>(value & 0x03);
  return true;
}

bool Pcf8563x::SetClockOut(const ClockOutConfig& config) {
  if (static_cast<uint8_t>(config.frequency) > 3) {
    return false;
  }
  return WriteRegister(Register::kClkoutControl,
      static_cast<uint8_t>((config.enabled ? 0x80 : 0) |
                           static_cast<uint8_t>(config.frequency)));
}

bool Pcf8563x::GetTimer(TimerConfig& config) {
  uint8_t data[2] = {};
  if (!ReadRegister(Register::kTimerControl, data, sizeof(data))) {
    return false;
  }
  config.enabled = (data[0] & 0x80) != 0;
  config.frequency = static_cast<TimerFrequency>(data[0] & 0x03);
  config.value = data[1];
  return true;
}

bool Pcf8563x::SetTimer(const TimerConfig& config) {
  const uint8_t frequency = static_cast<uint8_t>(config.frequency);
  if (frequency > 3 || (config.enabled && config.value == 0)) {
    return false;
  }
  if (!WriteRegister(Register::kTimerControl, frequency) ||
      !WriteRegister(Register::kTimer, config.value)) {
    return false;
  }
  return !config.enabled ||
         WriteRegister(Register::kTimerControl,
             static_cast<uint8_t>(0x80 | frequency));
}

bool Pcf8563x::StopTimer() {
  uint8_t control = 0;
  return ReadRegister(Register::kTimerControl, &control) &&
         WriteRegister(Register::kTimerControl,
             static_cast<uint8_t>(control & 0x03));
}

bool Pcf8563x::UpdateControlStatus2(
    uint8_t mask, uint8_t value, uint8_t clear_flags) {
  uint8_t control = 0;
  if (!ReadRegister(Register::kControlStatus2, &control)) {
    return false;
  }
  // AF/TF 写一保留、写零清除，避免读改写期间新产生的另一个事件丢失。
  const uint8_t result = ((control & 0x13) & ~mask) | (value & mask) |
                         (0x0C & ~clear_flags);
  return WriteRegister(Register::kControlStatus2, result);
}

bool Pcf8563x::SetTimerInterrupt(bool enabled, TimerInterruptMode mode) {
  if (mode != TimerInterruptMode::kLevel && mode != TimerInterruptMode::kPulse) {
    return false;
  }
  return UpdateControlStatus2(0x11,
      (enabled ? 0x01 : 0) | (mode == TimerInterruptMode::kPulse ? 0x10 : 0));
}

bool Pcf8563x::ClearTimerFlag() {
  return UpdateControlStatus2(0, 0, 0x04);
}

bool Pcf8563x::GetAlarm(Alarm& alarm) {
  uint8_t data[4] = {};
  if (!ReadRegister(Register::kMinuteAlarm, data, sizeof(data))) {
    return false;
  }
  Alarm result;
  result.minute_enabled = (data[0] & 0x80) == 0;
  result.hour_enabled = (data[1] & 0x80) == 0;
  result.day_enabled = (data[2] & 0x80) == 0;
  result.week_enabled = (data[3] & 0x80) == 0;
  if ((result.minute_enabled &&
          !DecodeBcd(data[0] & 0x7F, 0, 59, result.minute)) ||
      (result.hour_enabled &&
          !DecodeBcd(data[1] & 0x3F, 0, 23, result.hour)) ||
      (result.day_enabled && !DecodeBcd(data[2] & 0x3F, 1, 31, result.day)) ||
      (result.week_enabled && (data[3] & 0x07) > 6)) {
    return false;
  }
  if (result.week_enabled) {
    result.week = static_cast<Week>(data[3] & 0x07);
  }
  alarm = result;
  return true;
}

bool Pcf8563x::SetAlarm(const Alarm& alarm) {
  if ((alarm.minute_enabled && alarm.minute > 59) ||
      (alarm.hour_enabled && alarm.hour > 23) ||
      (alarm.day_enabled && (alarm.day < 1 || alarm.day > 31)) ||
      (alarm.week_enabled && static_cast<uint8_t>(alarm.week) > 6)) {
    return false;
  }
  uint8_t control = 0;
  if (!ReadRegister(Register::kControlStatus2, &control) ||
      !SetAlarmInterrupt(false)) {
    return false;
  }
  const uint8_t data[] = {
      alarm.minute_enabled ? EncodeBcd(alarm.minute) : uint8_t{0x80},
      alarm.hour_enabled ? EncodeBcd(alarm.hour) : uint8_t{0x80},
      alarm.day_enabled ? EncodeBcd(alarm.day) : uint8_t{0x80},
      alarm.week_enabled ? static_cast<uint8_t>(alarm.week) : uint8_t{0x80}};
  const bool written = WriteRegister(Register::kMinuteAlarm, data, sizeof(data));
  const bool restored = SetAlarmInterrupt((control & 0x02) != 0);
  return written && restored;
}

bool Pcf8563x::SetAlarmInterrupt(bool enabled) {
  return UpdateControlStatus2(0x02, enabled ? 0x02 : 0);
}

bool Pcf8563x::ClearAlarmFlag() {
  return UpdateControlStatus2(0, 0, 0x08);
}

bool Pcf8563x::ReadRegister(Register reg, uint8_t* data, size_t length) {
  const uint8_t address = static_cast<uint8_t>(reg);
  if (bus_ != nullptr && data != nullptr && length > 0 &&
      length <= static_cast<size_t>(16 - address) &&
      bus_->WriteRead(&address, 1, data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "PCF8563 register read failed (register: %#X, size: %zu)\n",
      static_cast<unsigned>(address), length);
  return false;
}

bool Pcf8563x::WriteRegister(Register reg, uint8_t value) {
  return WriteRegister(reg, &value, 1);
}

bool Pcf8563x::WriteRegister(Register reg, const uint8_t* data, size_t length) {
  const uint8_t address = static_cast<uint8_t>(reg);
  if (bus_ != nullptr && data != nullptr && length > 0 &&
      length <= static_cast<size_t>(16 - address)) {
    uint8_t packet[17] = {address};
    std::memcpy(packet + 1, data, length);
    if (bus_->Write(packet, length + 1)) {
      return true;
    }
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "PCF8563 register write failed (register: %#X, size: %zu)\n",
      static_cast<unsigned>(address), length);
  return false;
}

}  // namespace cpp_bus_driver
