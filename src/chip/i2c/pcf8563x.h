
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-17 14:02:37
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Pcf8563x final : public ChipI2cGuide {
 public:
  enum class Week {
    kSunday = 0x00,
    kMonday,
    kTuesday,
    kWednesday,
    kThursday,
    kFriday,
    kSaturday,
  };

  enum class OutFreq {
    kClockOff,
    kClock1Hz,
    kClock32Hz,
    kClock1024Hz,
    kClock32768Hz,
  };

  enum class TimerFreq {
    kClock4096Hz = 0,
    kClock64Hz,
    kClock1Hz,
    kClock1DividedBy60Hz,
  };

  struct TimeAlarm {
    struct {
      uint8_t value = 0;        // 分钟报警值（0~59）
      bool alarm_flag = false;  // 报警启用标志
    } minute;

    struct {
      uint8_t value = 0;        // 小时报警值（0~23）
      bool alarm_flag = false;  // 报警启用标志
    } hour;
    struct {
      uint8_t value = 0;        // 天报警值（1~31）
      bool alarm_flag = false;  // 报警启用标志
    } day;

    struct {
      Week value = Week::kSunday;  // 周报警值，使用Week::配置
      bool alarm_flag = false;     // 报警启用标志
    } week;
  };

  struct Time {
    uint8_t second = -1;
    uint8_t minute = -1;
    uint8_t hour = -1;
    uint8_t day = -1;
    Week week = Week::kSunday;
    uint8_t month = -1;
    uint8_t year = -1;
  };

  explicit Pcf8563x(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  uint8_t GetDeviceId();
  /**
   * @brief 设置CLKOUT引脚输出频率
   * @param freq_hz 使用Clock_Frequency::配置
   * @return
   * @Date 2025-02-27 15:46:00
   */
  bool SetClockFrequencyOutput(OutFreq freq_hz);

  /**
   * @brief 设置时钟启动
   *  @param enable [true]：启动，[false]：关闭
   * @return
   * @Date 2025-02-27 15:50:50
   */
  bool SetClock(bool enalbe);

  /**
   * @brief 检查时钟数据完整性
   * @return [0]：不保证时钟信息的完整 [1]：保证时钟完整
   * @Date 2025-02-27 15:35:07
   */
  bool CheckClockIntegrityFlag();

  /**
   * @brief 清除时钟数据完整性标志，设置0为时钟完整
   * @return
   * @Date 2025-02-27 15:36:50
   */
  bool ClearClockIntegrityFlag();

  uint8_t GetSecond();
  uint8_t GetMinute();
  uint8_t GetHour();
  uint8_t GetDay();
  uint8_t GetWeek();
  uint8_t GetMonth();
  uint8_t GetYear();
  bool GetTime(Time& time);

  bool SetSecond(uint8_t second);
  bool SetMinute(uint8_t minute);
  bool SetHour(uint8_t hour);
  bool SetDay(uint8_t day);
  bool SetWeek(Week week);
  bool SetMonth(uint8_t month);
  bool SetYear(uint8_t year);
  bool SetTime(Time time);

  /**
   * @brief 停止定时器
   * @return
   * @Date 2025-02-27 16:02:39
   */
  bool StopTimer();

  /**
   * @brief 开启定时器
   * @param n_value 定时器值，与freq搭配使用（定时的值 （单位：秒） = n_value /
   * freq（单位：赫兹））
   * @param freq_hz 使用Timer_Freq::配置，选择的频率越高定时的时间越精准
   * @return
   * @Date 2025-02-27 16:12:48
   */
  bool RunTimer(uint8_t n_value, TimerFreq freq_hz);

  /**
   * @brief 检查定时器标志和中断
   * @return
   * @Date 2025-02-27 16:53:51
   */
  bool CheckTimerFlag();

  /**
   * @brief 清除定时器标志和中断
   * @return
   * @Date 2025-02-27 16:54:04
   */
  bool ClearTimerFlag();

  /**
   * @brief 停止预定时间报警
   * @return
   * @Date 2025-02-27 17:30:19
   */
  bool StopScheduledAlarm();

  /**
   * @brief 开启预定时间报警
   * @param alarm 使用Time_Alarm::配置
   * @return
   * @Date 2025-02-27 17:31:35
   */
  bool RunScheduledAlarm(TimeAlarm alarm);

  /**
   * @brief 检查预定时间报警标志和中断
   * @return
   * @Date 2025-02-27 17:55:50
   */
  bool CheckScheduledAlarmFlag();

  /**
   * @brief 清除预定时间报警标志和中断
   * @return
   * @Date 2025-02-27 17:56:56
   */
  bool ClearScheduledAlarmFlag();

 private:
  enum class Cmd {
    kRoDeviceId = 0x00,

    kRwControlStatus1 = 0x00,
    kRwControlStatus2,
    kRwVlSeconds,
    kRwMinutes,
    kRwHours,
    kRwDays,
    kRwWeekdays,
    kRwCenturyMonths,
    kRwYears,
    kRwMinuteAlarm,
    kRwHourAlarm,
    kRwDayAlarm,
    kRwWeekdayAlarm,
    kRwClkoutControl,
    kRwTimerControl,
    kRwTimer,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x51;
  static constexpr uint8_t kInitSequence[] = {
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwClkoutControl), 0B00000000};

  int32_t rst_;
};
}  // namespace cpp_bus_driver