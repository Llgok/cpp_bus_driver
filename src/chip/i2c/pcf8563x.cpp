/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:51
 * @LastEditTime: 2026-04-20 15:08:31
 * @License: GPL 3.0
 */
#include "pcf8563x.h"

namespace cpp_bus_driver {
bool Pcf8563x::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetPinMode(rst_, PinMode::kOutput, PinStatus::kPullup);

    PinWrite(rst_, 1);
    DelayMs(10);
    PinWrite(rst_, 0);
    DelayMs(10);
    PinWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer == static_cast<uint8_t>(CPP_BUS_DRIVER_DEFAULT_VALUE)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get pcf8563x id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get pcf8563x id success (id: %#X)\n", buffer);
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  ClearClockIntegrityFlag();

  return true;
}

uint8_t Pcf8563x::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Pcf8563x::SetClockFrequencyOutput(OutFreq freq_hz) {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwClkoutControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  switch (freq_hz) {
    case OutFreq::kClockOff:
      buffer &= 0B01111111;
      break;
    case OutFreq::kClock1Hz:
      buffer |= 0B10000011;
      break;
    case OutFreq::kClock32Hz:
      buffer = (buffer & 0B01111100) | 0B10000010;
      break;
    case OutFreq::kClock1024Hz:
      buffer = (buffer & 0B01111100) | 0B10000001;
      break;
    case OutFreq::kClock32768Hz:
      buffer = (buffer & 0B01111100) | 0B10000000;
      break;

    default:
      break;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwClkoutControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::SetClock(bool enalbe) {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwControlStatus1), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  if (enalbe) {
    buffer &= 0B11011111;  // 开启时钟
  } else {
    buffer |= 0B00100000;  // 停止时钟
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatus1), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  return true;
}

bool Pcf8563x::CheckClockIntegrityFlag() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwVlSeconds), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return !((buffer & 0B10000000) >> 7);
}

bool Pcf8563x::ClearClockIntegrityFlag() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwVlSeconds), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer &= 0B01111111;

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwVlSeconds), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint8_t Pcf8563x::GetSecond() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwVlSeconds), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (((buffer & 0B01110000) >> 4) * 10) + (buffer & 0B00001111);
}

uint8_t Pcf8563x::GetMinute() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwMinutes), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (((buffer & 0B01110000) >> 4) * 10) + (buffer & 0B00001111);
}

uint8_t Pcf8563x::GetHour() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwHours), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (((buffer & 0B00110000) >> 4) * 10) + (buffer & 0B00001111);
}

uint8_t Pcf8563x::GetDay() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDays), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (((buffer & 0B00110000) >> 4) * 10) + (buffer & 0B00001111);
}

uint8_t Pcf8563x::GetWeek() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwWeekdays), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (buffer & 0B00000111);
}

uint8_t Pcf8563x::GetMonth() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwCenturyMonths), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (((buffer & 0B00010000) >> 4) * 10) + (buffer & 0B00001111);
}

uint8_t Pcf8563x::GetYear() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwYears), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (((buffer & 0B11110000) >> 4) * 10) + (buffer & 0B00001111);
}

bool Pcf8563x::GetTime(Time& time) {
  uint8_t buffer[7] = {0};

  for (uint8_t i = 0; i < 7; i++) {
    if (!bus_->Read(
            static_cast<uint8_t>(static_cast<uint8_t>(Cmd::kRwVlSeconds) + i),
            &buffer[i])) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
  }

  time.second =
      (((buffer[0] & 0B01110000) >> 4) * 10) + (buffer[0] & 0B00001111);
  time.minute =
      (((buffer[1] & 0B01110000) >> 4) * 10) + (buffer[1] & 0B00001111);
  time.hour = (((buffer[2] & 0B00110000) >> 4) * 10) + (buffer[2] & 0B00001111);
  time.day = (((buffer[3] & 0B00110000) >> 4) * 10) + (buffer[3] & 0B00001111);
  time.week = static_cast<Week>(buffer[4] & 0B00000111);
  time.month =
      (((buffer[5] & 0B00010000) >> 4) * 10) + (buffer[5] & 0B00001111);
  time.year = (((buffer[6] & 0B11110000) >> 4) * 10) + (buffer[6] & 0B00001111);

  return true;
}

bool Pcf8563x::SetSecond(uint8_t second) {
  if (second > 59) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    second = 59;
  }

  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwVlSeconds), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B10000000) | (((second / 10) << 4) | (second % 10));

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwVlSeconds), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::SetMinute(uint8_t minute) {
  if (minute > 59) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    minute = 59;
  }

  uint8_t buffer = (((minute / 10) << 4) | (minute % 10));

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwMinutes), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::SetHour(uint8_t hour) {
  if (hour > 23) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    hour = 23;
  }

  uint8_t buffer = (((hour / 10) << 4) | (hour % 10));

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwHours), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::SetDay(uint8_t day) {
  if (day > 31) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    day = 31;
  } else if (day < 1) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    day = 1;
  }

  uint8_t buffer = (((day / 10) << 4) | (day % 10));

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwDays), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::SetWeek(Week week) {
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwWeekdays), static_cast<uint8_t>(week))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::SetMonth(uint8_t month) {
  if (month > 12) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    month = 12;
  } else if (month < 1) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    month = 1;
  }

  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwCenturyMonths), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B11100000) | (((month / 10) << 4) | (month % 10));

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwCenturyMonths), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::SetYear(uint8_t year) {
  if (year > 99) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    year = 99;
  }

  uint8_t buffer = (((year / 10) << 4) | (year % 10));

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwYears), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::SetTime(Time time) {
  if (time.second > 59) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    time.second = 59;
  }
  if (time.minute > 59) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    time.minute = 59;
  }
  if (time.hour > 23) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    time.hour = 23;
  }
  if (time.day > 31) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    time.day = 31;
  } else if (time.day < 1) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    time.day = 1;
  }
  if (time.month > 12) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    time.month = 12;
  } else if (time.month < 1) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    time.month = 1;
  }
  if (time.year > 99) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    time.year = 99;
  }

  uint8_t buffer[7] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwVlSeconds), &buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwCenturyMonths), &buffer[5])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer[0] = (buffer[0] & 0B10000000) |
              (((time.second / 10) << 4) | (time.second % 10));
  buffer[1] = (((time.minute / 10) << 4) | (time.minute % 10));
  buffer[2] = (((time.hour / 10) << 4) | (time.hour % 10));
  buffer[3] = (((time.day / 10) << 4) | (time.day % 10));
  buffer[4] = static_cast<uint8_t>(time.week);
  buffer[5] =
      (buffer[5] & 0B11100000) | (((time.month / 10) << 4) | (time.month % 10));
  buffer[6] = (((time.year / 10) << 4) | (time.year % 10));

  for (uint8_t i = 0; i < 7; i++) {
    if (!bus_->Write(
            static_cast<uint8_t>(static_cast<uint8_t>(Cmd::kRwVlSeconds) + i),
            buffer[i])) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  return true;
}

bool Pcf8563x::StopTimer() {
  uint8_t buffer = 0;

  // 关闭定时器
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwTimerControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 清除定时器TF标志位并关闭定时器TIE外部中断
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwControlStatus2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer &= 0B11111010;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatus2), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  return true;
}

bool Pcf8563x::RunTimer(uint8_t n_value, TimerFreq freq_hz) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwTimer), n_value)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  uint8_t buffer = 0B10000000 | static_cast<uint8_t>(freq_hz);

  // 开启定时器并设置频率
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwTimerControl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 清除定时器TF标志位并开启定时器TIE外部中断
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwControlStatus2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111010) | 0B00000001;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatus2), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::CheckTimerFlag() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwControlStatus2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0B00000100) >> 2;
}

bool Pcf8563x::ClearTimerFlag() {
  uint8_t buffer = 0;
  // 清除定时器TF标志位
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwControlStatus2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer &= 0B11111011;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatus2), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::StopScheduledAlarm() {
  uint8_t buffer = 0B10000000;

  // 关闭报警
  for (uint8_t i = 0; i < 4; i++) {
    if (!bus_->Write(
            static_cast<uint8_t>(static_cast<uint8_t>(Cmd::kRwMinuteAlarm) + i),
            buffer)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  // 清除报警AF标志位并关闭报警AIE外部中断
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwControlStatus2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer &= 0B11110101;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatus2), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::RunScheduledAlarm(TimeAlarm alarm) {
  uint8_t buffer = 0;

  if (alarm.minute.alarm_flag) {
    if (alarm.minute.value > 59) {
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      alarm.minute.value = 59;
    }
    buffer = (((alarm.minute.value / 10) << 4) | (alarm.minute.value % 10));
  } else {
    buffer = 0B10000000;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwMinuteAlarm), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (alarm.hour.alarm_flag) {
    if (alarm.hour.value > 23) {
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      alarm.hour.value = 23;
    }
    buffer = (((alarm.hour.value / 10) << 4) | (alarm.hour.value % 10));
  } else {
    buffer = 0B10000000;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwHourAlarm), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (alarm.day.alarm_flag) {
    if (alarm.day.value > 31) {
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      alarm.day.value = 31;
    } else if (alarm.day.value < 1) {
      alarm.day.value = 1;
    }
    buffer = (((alarm.day.value / 10) << 4) | (alarm.day.value % 10));
  } else {
    buffer = 0B10000000;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwDayAlarm), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (alarm.week.alarm_flag) {
    buffer = static_cast<uint8_t>(alarm.week.value);
  } else {
    buffer = 0B10000000;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwWeekdayAlarm), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 清除预定时间报警AF标志位并开启预定时间报警AIE外部中断
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwControlStatus2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11110101) | 0B00000010;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatus2), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Pcf8563x::CheckScheduledAlarmFlag() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwControlStatus2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (buffer & 0B00001000) >> 3;
}

bool Pcf8563x::ClearScheduledAlarmFlag() {
  uint8_t buffer = 0;
  // 清除预定时间报警AF标志位
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwControlStatus2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer &= 0B11110111;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatus2), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
