/*
 * @Description: PCF8563 系列实时时钟芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-09-10 14:37:20
 * @LastEditTime: 2026-09-10 14:50:55
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {

// 多事务配置由调用方串行化；初始化和释放总线均不改变日历及中断配置。
class Pcf8563x final : public I2cChipBase {
 public:
  enum class Week : uint8_t {
    kSunday = 0,
    kMonday = 1,
    kTuesday = 2,
    kWednesday = 3,
    kThursday = 4,
    kFriday = 5,
    kSaturday = 6,
  };

  enum class ClockOutFrequency : uint8_t {
    k32768Hz = 0,
    k1024Hz = 1,
    k32Hz = 2,
    k1Hz = 3,
  };

  enum class TimerFrequency : uint8_t {
    k4096Hz = 0,
    k64Hz = 1,
    k1Hz = 2,
    k1PerMinute = 3,
  };

  enum class TimerInterruptMode : uint8_t {
    kLevel = 0,
    kPulse = 1,
  };

  struct Time {
    uint8_t second = 0;
    uint8_t minute = 0;
    uint8_t hour = 0;
    uint8_t day = 1;
    Week week = Week::kSaturday;
    uint8_t month = 1;
    uint8_t year = 0;
    // 原始 C 位，年份 99 -> 00 时翻转；世纪含义由应用约定。
    bool century = false;
  };

  struct Alarm {
    uint8_t minute = 0;
    uint8_t hour = 0;
    uint8_t day = 1;
    Week week = Week::kSunday;
    bool minute_enabled = false;
    bool hour_enabled = false;
    bool day_enabled = false;
    bool week_enabled = false;
  };

  struct Status {
    bool clock_stopped = false;
    bool voltage_low = false;
    bool alarm_flag = false;
    bool timer_flag = false;
    bool alarm_interrupt_enabled = false;
    bool timer_interrupt_enabled = false;
    TimerInterruptMode timer_interrupt_mode = TimerInterruptMode::kLevel;
  };

  struct ClockOutConfig {
    bool enabled = false;
    ClockOutFrequency frequency = ClockOutFrequency::k32768Hz;
  };

  struct TimerConfig {
    bool enabled = false;
    TimerFrequency frequency = TimerFrequency::k1Hz;
    // 读取时为当前倒计数值；配置启动时应为 1~255。
    uint8_t value = 0;
  };

  explicit Pcf8563x(std::shared_ptr<I2cBusBase> bus, int16_t address = 0x51)
      : I2cChipBase(bus, address) {}

  bool Init(int32_t freq_hz = 100000) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取控制状态及低电压标志
   * @param status 接收状态，仅通信成功时更新
   * @return 通信成功返回 true，否则返回 false
   */
  bool GetStatus(Status& status);

  /**
   * @brief 设置 STOP 位，控制日历计时
   * @param enabled true 运行，false 停止
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetClockEnabled(bool enabled);

  /**
   * @brief 连续读取日历寄存器，避免跨秒拼接
   * @param time 接收日期时间，仅有效 BCD 和日期读取成功时更新
   * @param voltage_low 接收 VL 位；true 时即使日期合法也不能保证时间可靠
   * @return 通信和格式检查成功返回 true，否则返回 false
   * @note 总线事务必须在一秒内完成。年份按芯片规则每四年闰年。
   */
  bool GetTime(Time& time, bool& voltage_low);

  /**
   * @brief 连续写入完整日期时间，同时清除 VL 位
   * @param time 日期时间，year 为 0~99，century 为原始 C 位
   * @return 写入成功返回 true；参数无效时不访问总线
   * @note 不改变 STOP 状态；通信失败可能已有部分寄存器写入。
   * 芯片不处理公历整百年例外，应用需处理 2100 年等年份。
   */
  bool SetTime(const Time& time);

  /**
   * @brief 读取 CLKOUT 配置
   * @param config 接收配置，仅操作成功时更新
   * @return 操作成功返回 true，否则返回 false
   */
  bool GetClockOut(ClockOutConfig& config);
  /**
   * @brief 配置 CLKOUT 使能及频率
   * @param config 时钟输出配置，非法频率不访问总线
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetClockOut(const ClockOutConfig& config);
  /**
   * @brief 读取倒计时配置及当前计数
   * @param config 接收配置，仅操作成功时更新
   * @return 操作成功返回 true，否则返回 false
   */
  bool GetTimer(TimerConfig& config);
  /**
   * @brief 停止旧倒计时、写入计数与频率，并按配置启动
   * @param config 倒计时配置，启动时 value 不得为零
   * @return 配置成功返回 true，否则返回 false
   * @note 不改变 TIE、TI_TP 或 TF，失败时倒计时可能已停止。
   */
  bool SetTimer(const TimerConfig& config);
  /**
   * @brief 停止倒计时，保留频率、TF 及中断配置
   * @return 操作成功返回 true，否则返回 false
   */
  bool StopTimer();
  /**
   * @brief 配置定时器中断
   * @param enabled 是否启用 TIE
   * @param mode TI_TP 对应的电平或脉冲模式
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetTimerInterrupt(bool enabled, TimerInterruptMode mode);
  /**
   * @brief 清除 TF，保留其他控制位和 AF
   * @return 操作成功返回 true，否则返回 false
   */
  bool ClearTimerFlag();
  /**
   * @brief 读取闹钟比较字段及字段使能
   * @param alarm 接收配置，仅操作成功时更新；禁用字段返回默认值
   * @return 操作成功返回 true，否则返回 false
   */
  bool GetAlarm(Alarm& alarm);
  /**
   * @brief 配置闹钟比较字段，全部禁用表示关闭比较
   * @param alarm 仅对使能字段验证数值
   * @return 配置成功返回 true，否则返回 false
   * @note 配置期间暂时关闭 AIE，随后恢复；保留 AF、TF。
   * 失败时可能已部分写入，调用方应重新配置。
   */
  bool SetAlarm(const Alarm& alarm);
  /**
   * @brief 设置 AIE，保留中断标志和定时器控制
   * @param enabled 是否启用闹钟中断
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetAlarmInterrupt(bool enabled);
  /**
   * @brief 清除 AF，保留其他控制位和 TF
   * @return 操作成功返回 true，否则返回 false
   */
  bool ClearAlarmFlag();

 private:
  // PCF8563 寄存器地址。
  enum class Register : uint8_t {
    kControlStatus1 = 0x00,
    kControlStatus2 = 0x01,
    kVlSeconds = 0x02,
    kMinutes = 0x03,
    kHours = 0x04,
    kDays = 0x05,
    kWeekdays = 0x06,
    kCenturyMonths = 0x07,
    kYears = 0x08,
    kMinuteAlarm = 0x09,
    kHourAlarm = 0x0A,
    kDayAlarm = 0x0B,
    kWeekdayAlarm = 0x0C,
    kClkoutControl = 0x0D,
    kTimerControl = 0x0E,
    kTimer = 0x0F,
  };

  /**
   * @brief 更新状态控制位，并仅清除指定事件标志
   * @param mask 待更新的 TIE、AIE、TI_TP 位掩码
   * @param value 控制位的新值
   * @param clear_flags 待清除的 AF、TF 位掩码，零表示全部保留
   * @return 操作成功返回 true，否则返回 false
   */
  bool UpdateControlStatus2(uint8_t mask, uint8_t value,
      uint8_t clear_flags = 0);
  /**
   * @brief 连续读取寄存器并集中记录访问错误
   * @param reg 起始寄存器地址
   * @param data 接收缓冲区，通信失败时可能已部分更新
   * @param length 读取长度
   * @return 读取成功返回 true，否则返回 false
   */
  bool ReadRegister(Register reg, uint8_t* data, size_t length = 1);
  /**
   * @brief 写入单个寄存器
   * @param reg 寄存器地址
   * @param value 写入值
   * @return 写入成功返回 true，否则返回 false
   */
  bool WriteRegister(Register reg, uint8_t value);
  /**
   * @brief 连续写入寄存器并集中记录访问错误
   * @param reg 起始寄存器地址
   * @param data 待写入数据
   * @param length 写入长度
   * @return 写入成功返回 true，否则返回 false
   */
  bool WriteRegister(Register reg, const uint8_t* data, size_t length);
};

}  // namespace cpp_bus_driver
