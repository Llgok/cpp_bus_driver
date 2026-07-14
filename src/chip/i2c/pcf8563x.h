/*
 * @Description: PCF8563 系列实时时钟芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:43:38
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
      int32_t rst = kDefaultValue)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit(bool delete_bus = true) override;
  uint8_t GetDeviceId();
  /**
   * @brief 设置CLKOUT引脚输出频率
   * @param freq_hz 时钟输出频率
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetClockFrequencyOutput(OutFreq freq_hz);

  /**
   * @brief 设置时钟启动
   * @param enalbe true 表示启动，false 表示停止
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetClock(bool enalbe);

  /**
   * @brief 检查时钟数据完整性
   * @return [0]：不保证时钟信息的完整 [1]：保证时钟完整
   */
  bool CheckClockIntegrityFlag();

  /**
   * @brief 清除时钟数据完整性标志，设置0为时钟完整
   * @return 操作成功返回 true，失败返回 false
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
   * @return 操作成功返回 true，失败返回 false
   */
  bool StopTimer();

  /**
   * @brief 开启定时器
   * @param n_value 定时器值，与freq搭配使用（定时的值 （单位：秒） = n_value /
   * freq（单位：赫兹））
   * @param freq_hz 定时器时钟频率；频率越高，定时精度越高
   * @return 操作成功返回 true，失败返回 false
   */
  bool RunTimer(uint8_t n_value, TimerFreq freq_hz);

  /**
   * @brief 检查定时器标志和中断
   * @return 操作成功返回 true，失败返回 false
   */
  bool CheckTimerFlag();

  /**
   * @brief 清除定时器标志和中断
   * @return 操作成功返回 true，失败返回 false
   */
  bool ClearTimerFlag();

  /**
   * @brief 停止预定时间报警
   * @return 操作成功返回 true，失败返回 false
   */
  bool StopScheduledAlarm();

  /**
   * @brief 开启预定时间报警
   * @param alarm 定时闹钟配置
   * @return 操作成功返回 true，失败返回 false
   */
  bool RunScheduledAlarm(TimeAlarm alarm);

  /**
   * @brief 检查预定时间报警标志和中断
   * @return 操作成功返回 true，失败返回 false
   */
  bool CheckScheduledAlarmFlag();

  /**
   * @brief 清除预定时间报警标志和中断
   * @return 操作成功返回 true，失败返回 false
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
