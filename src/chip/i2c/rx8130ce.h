/*
 * @Description: RX8130CE 实时时钟芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-10-31 11:00:00
 * @LastEditTime: 2026-10-31 16:41:05
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {

class Rx8130ce final : public I2cChipBase {
 public:
  enum class Weekday : uint8_t {
    kSunday = 0,
    kMonday,
    kTuesday,
    kWednesday,
    kThursday,
    kFriday,
    kSaturday,
  };

  enum class ClockOutput : uint8_t {
    k32768Hz = 0,
    k1024Hz = 1,
    k1Hz = 2,
    kDisabled = 3,
  };

  enum class TimerClock : uint8_t {
    k4096Hz = 0,
    k64Hz = 1,
    k1Hz = 2,
    k1PerMinute = 3,
    k1PerHour = 4,
  };

  enum class ResetVoltage : uint8_t {
    k2750mV = 0,
    k2650mV = 1,
  };

  enum class BatteryFullVoltage : uint8_t {
    k3020mV = 0,
    k3080mV = 1,
    k2920mV = 2,
    kDisabled = 3,
  };

  struct Time {
    uint8_t second = 0;
    uint8_t minute = 0;
    uint8_t hour = 0;
    Weekday weekday = Weekday::kSunday;
    uint8_t day = 1;
    uint8_t month = 1;
    uint8_t year = 0;
  };

  struct Status {
    bool battery_low = false;
    bool update_flag = false;
    bool timer_flag = false;
    bool alarm_flag = false;
    bool reset_flag = false;
    bool voltage_loss = false;
    bool battery_full = false;
    bool charger_enabled = false;
    bool power_switch_enabled = false;
    bool update_interrupt_enabled = false;
    bool timer_interrupt_enabled = false;
    bool alarm_interrupt_enabled = false;
    bool timer_enabled = false;
    bool clock_stopped = false;
    ClockOutput clock_output = ClockOutput::k32768Hz;
  };

  struct Alarm {
    uint8_t minute = 0;
    uint8_t hour = 0;
    uint8_t day_or_weekday = 0;
    bool minute_enabled = false;
    bool hour_enabled = false;
    bool day_or_weekday_enabled = false;
    bool weekday_mode = false;
  };

  struct Timer {
    uint16_t counter = 0;
    TimerClock clock = TimerClock::k1Hz;
    bool enabled = false;
  };

  explicit Rx8130ce(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDefaultI2cAddress)
      : I2cChipBase(bus, address) {}

  bool Init(int32_t freq_hz = kDefaultI2cFrequencyHz) override;

  /**
   * @brief 按手册执行软件复位序列
   * 软件复位会触发芯片的 /RST 输出并置位 VLF，不会清除已有日历数据；
   * 仅在调用方确认需要异常恢复时使用，普通初始化不会自动调用。
   * 调用前须完成总线初始化并满足上电等待时间；成功后已等待 125 ms，
   * 调用方需重新配置电源切换等控制位，并在确认时间可靠后清除 VLF。
   * @return 复位成功返回 true，否则返回 false
   */
  bool SoftwareReset();

  /**
   * @brief 释放芯片对应的 I2C 设备
   * @param delete_bus true 时同时请求释放总线
   * @return 释放成功返回 true，否则返回 false
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 连续读取并校验完整日历数据
   * @param time 接收日期时间，仅成功时更新
   * @return 寄存器数据有效且读取成功返回 true
   */
  bool GetTime(Time& time);

  /**
   * @brief 设置完整日期时间并在写入期间暂停时钟更新
   * @param time 日期时间，年份范围为 2000~2099
   * @return 参数有效且写入成功返回 true
   */
  bool SetTime(const Time& time);

  /**
   * @brief 读取状态、控制位和中断配置
   * @param status 接收状态，仅成功时更新
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetStatus(Status& status);

  /**
   * @brief 清除时钟振荡停止标志 VLF
   * @return 操作成功返回 true，否则返回 false
   */
  bool ClearVoltageLossFlag();

  /**
   * @brief 清除 VBAT 低电压标志 VBLF
   * @return 操作成功返回 true，否则返回 false
   */
  bool ClearBatteryLowFlag();

  /**
   * @brief 清除复位检测标志 RSF
   * @return 操作成功返回 true，否则返回 false
   */
  bool ClearResetFlag();

  /**
   * @brief 设置 VBAT 自动切换功能 INIEN
   * @param enabled true 启用自动电源切换
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetPowerSwitchEnabled(bool enabled);

  /**
   * @brief 读取 VBAT 自动切换功能 INIEN
   * @param enabled 接收使能状态，仅成功时更新
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetPowerSwitchEnabled(bool& enabled);

  /**
   * @brief 设置 VBAT 充电功能 CHGEN
   * @param enabled true 启用芯片内部充电通路
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetChargerEnabled(bool enabled);

  /**
   * @brief 读取 VBAT 充电功能 CHGEN
   * @param enabled 接收使能状态，仅成功时更新
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetChargerEnabled(bool& enabled);

  /**
   * @brief 设置 VDD 复位检测阈值
   * @param voltage 2.75 V 或 2.65 V
   * @return 参数有效且写入成功返回 true
   */
  bool SetResetVoltage(ResetVoltage voltage);

  /**
   * @brief 读取 VDD 复位检测阈值
   * @param voltage 接收阈值，仅成功时更新
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetResetVoltage(ResetVoltage& voltage);

  /**
   * @brief 设置电池满电检测阈值
   * @param voltage 3.02 V、3.08 V、2.92 V 或关闭检测
   * @return 参数有效且写入成功返回 true
   */
  bool SetBatteryFullVoltage(BatteryFullVoltage voltage);

  /**
   * @brief 读取电池满电检测阈值
   * @param voltage 接收阈值，仅成功时更新
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetBatteryFullVoltage(BatteryFullVoltage& voltage);

  /**
   * @brief 设置 VBAT 低电压检测功能 VBLFE
   * @param enabled true 开启检测
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetBatteryLowDetectionEnabled(bool enabled);

  /**
   * @brief 读取 VBAT 低电压检测功能 VBLFE
   * @param enabled 接收使能状态，仅成功时更新
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetBatteryLowDetectionEnabled(bool& enabled);

  /**
   * @brief 设置 FOUT 输出频率
   * @param output 输出频率或关闭
   * @return 参数有效且写入成功返回 true
   */
  bool SetClockOutput(ClockOutput output);

  /**
   * @brief 读取 FOUT 输出频率
   * @param output 接收频率，仅成功时更新
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetClockOutput(ClockOutput& output);

  /**
   * @brief 配置闹钟比较值并保留原中断使能状态
   * @param alarm 闹钟日期/星期和字段使能
   * @return 参数有效且寄存器操作成功返回 true
   */
  bool SetAlarm(const Alarm& alarm);

  /**
   * @brief 读取闹钟比较值和字段使能状态
   * @param alarm 接收配置，仅成功时更新
   * @return 读取成功且数据有效返回 true
   */
  bool GetAlarm(Alarm& alarm);

  /**
   * @brief 设置闹钟中断使能 AIE
   * @param enabled true 启用闹钟中断
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetAlarmInterruptEnabled(bool enabled);

  /**
   * @brief 清除闹钟标志 AF
   * @return 操作成功返回 true，否则返回 false
   */
  bool ClearAlarmFlag();

  /**
   * @brief 配置 16 位唤醒定时器和时钟源
   * @param timer 定时器计数、时钟源和启动状态
   * @return 参数有效且寄存器操作成功返回 true
   */
  bool SetTimer(const Timer& timer);

  /**
   * @brief 读取唤醒定时器当前配置
   * @param timer 接收定时器配置，仅成功时更新
   * @return 读取成功且时钟源有效返回 true
   */
  bool GetTimer(Timer& timer);

  /**
   * @brief 停止唤醒定时器
   * @return 操作成功返回 true，否则返回 false
   */
  bool StopTimer();

  /**
   * @brief 设置定时器中断使能 TIE
   * @param enabled true 启用定时器中断
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetTimerInterruptEnabled(bool enabled);

  /**
   * @brief 清除定时器标志 TF
   * @return 操作成功返回 true，否则返回 false
   */
  bool ClearTimerFlag();

  /**
   * @brief 设置时间更新中断使能 UIE
   * @param enabled true 启用时间更新中断
   * @return 操作成功返回 true，否则返回 false
   */
  bool SetUpdateInterruptEnabled(bool enabled);

  /**
   * @brief 清除时间更新标志 UF
   * @return 操作成功返回 true，否则返回 false
   */
  bool ClearUpdateFlag();

  /**
   * @brief 读取用户 RAM
   * @param data 接收缓冲区，长度为 1~4 字节
   * @param length 读取长度
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetUserRam(uint8_t* data, size_t length = kUserRamSize);

  /**
   * @brief 写入用户 RAM
   * @param data 待写入数据
   * @param length 写入长度，范围为 1~4 字节
   * @return 写入成功返回 true，否则返回 false
   */
  bool SetUserRam(const uint8_t* data, size_t length = kUserRamSize);

  /**
   * @brief 读取数字校准偏移寄存器 0x30
   * @param value 接收寄存器原始值
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetDigitalOffset(uint8_t& value);

  /**
   * @brief 写入数字校准偏移寄存器 0x30 原始值
   * @param value 寄存器原始值，bit0 为 DTE
   * @return 写入成功返回 true，否则返回 false
   */
  bool SetDigitalOffset(uint8_t value);

  /**
   * @brief 设置扩展寄存器 1 的 VBLFE 位
   * @param value 寄存器 bit0 的值
   * @return 写入成功返回 true，否则返回 false
   */
  bool SetExtension1(uint8_t value);

  /**
   * @brief 读取扩展寄存器 1
   * @param value 接收寄存器原始值
   * @return 读取成功返回 true，否则返回 false
   */
  bool GetExtension1(uint8_t& value);
 private:
  static constexpr int16_t kDefaultI2cAddress = 0x32;
  static constexpr int32_t kDefaultI2cFrequencyHz = 100000;
  static constexpr size_t kUserRamSize = 4;

  enum class Register : uint8_t {
    kSecond = 0x10,
    kMinute = 0x11,
    kHour = 0x12,
    kWeekday = 0x13,
    kDay = 0x14,
    kMonth = 0x15,
    kYear = 0x16,
    kAlarmMinute = 0x17,
    kAlarmHour = 0x18,
    kAlarmWeekdayDay = 0x19,
    kTimerCounter0 = 0x1A,
    kTimerCounter1 = 0x1B,
    kExtension = 0x1C,
    kFlag = 0x1D,
    kControl0 = 0x1E,
    kControl1 = 0x1F,
    kUserRam0 = 0x20,
    kUserRam1 = 0x21,
    kUserRam2 = 0x22,
    kUserRam3 = 0x23,
    kDigitalOffset = 0x30,
    kExtension1 = 0x31,
  };

  /**
   * @brief 读取手册定义的用户寄存器
   * @param address 起始寄存器地址
   * @param data 接收缓冲区
   * @param length 读取长度
   * @return 地址范围有效且读取成功返回 true
   */
  bool ReadRegister(Register address, uint8_t* data, size_t length = 1);

  /**
   * @brief 读取单个手册定义的用户寄存器
   * @param address 寄存器地址
   * @param value 接收寄存器值
   * @return 读取成功返回 true，否则返回 false
   */
  bool ReadRegister(Register address, uint8_t& value);

  /**
   * @brief 写入手册定义的用户寄存器
   * @param address 起始寄存器地址
   * @param data 待写入数据
   * @param length 写入长度
   * @return 地址范围有效且写入成功返回 true
   */
  bool WriteRegister(Register address, const uint8_t* data, size_t length);

  /**
   * @brief 写入单个手册定义的用户寄存器
   * @param address 寄存器地址
   * @param value 写入值
   * @return 写入成功返回 true，否则返回 false
   */
  bool WriteRegister(Register address, uint8_t value);

  /**
   * @brief 将十进制数编码为压缩 BCD
   * @param value 待编码值
   * @return BCD 编码值
   */
  static uint8_t EncodeBcd(uint8_t value);

  /**
   * @brief 校验并解码压缩 BCD
   * @param value BCD 原始值
   * @param maximum 允许的最大值
   * @param result 接收解码结果，仅成功时更新
   * @return 数据有效返回 true，否则返回 false
   */
  static bool DecodeBcd(uint8_t value, uint8_t maximum, uint8_t& result);

  /**
   * @brief 校验日期时间字段及月份天数
   * @param time 待校验日期时间
   * @return 所有字段有效返回 true，否则返回 false
   */
  static bool IsValidTime(const Time& time);

  /**
   * @brief 编码闹钟字段，未启用字段写入 AE 位
   * @param value 十进制字段值
   * @param enabled 字段是否启用
   * @return 寄存器编码值
   */
  static uint8_t AlarmValue(uint8_t value, bool enabled);

  /**
   * @brief 按掩码更新控制寄存器 0
   * @param mask 待更新位掩码
   * @param value 掩码范围内的新值
   * @return 读改写成功返回 true，否则返回 false
   */
  bool UpdateControl0(uint8_t mask, uint8_t value);

  /**
   * @brief 按掩码更新控制寄存器 1
   * @param mask 待更新位掩码
   * @param value 掩码范围内的新值
   * @return 读改写成功返回 true，否则返回 false
   */
  bool UpdateControl1(uint8_t mask, uint8_t value);

  /**
   * @brief 按掩码更新扩展寄存器
   * @param mask 待更新位掩码
   * @param value 掩码范围内的新值
   * @return 读改写成功返回 true，否则返回 false
   */
  bool UpdateExtension(uint8_t mask, uint8_t value);

  /**
   * @brief 按掩码更新扩展寄存器 1
   * @param mask 待更新位掩码
   * @param value 掩码范围内的新值
   * @return 读改写成功返回 true，否则返回 false
   */
  bool UpdateExtension1(uint8_t mask, uint8_t value);

  /**
   * @brief 按写零清除规则清除指定状态标志
   * @param mask 待清除标志掩码
   * @return 清除成功返回 true，否则返回 false
   */
  bool UpdateFlag(uint8_t mask);

  /**
   * @brief 检查寄存器范围是否属于手册定义的用户寄存器
   * @param address 起始寄存器地址
   * @param length 访问长度
   * @return 范围有效返回 true，否则返回 false
   */
  static bool IsUserRegisterRange(uint8_t address, size_t length);

  /**
   * @brief 读取单个 8 位寄存器
   * @param address 寄存器地址
   * @param value 接收寄存器值
   * @return 读取成功返回 true，否则返回 false
   */
  bool ReadRegister(uint8_t address, uint8_t& value);

  /**
   * @brief 读取连续的 8 位用户寄存器
   * @param address 起始寄存器地址
   * @param data 接收缓冲区
   * @param length 读取长度
   * @return 地址范围有效且读取成功返回 true，否则返回 false
   */
  bool ReadRegister(uint8_t address, uint8_t* data, size_t length);

  /**
   * @brief 写入单个 8 位寄存器
   * @param address 寄存器地址
   * @param value 写入值
   * @return 写入成功返回 true，否则返回 false
   */
  bool WriteRegister(uint8_t address, uint8_t value);

  /**
   * @brief 写入连续的 8 位用户寄存器
   * @param address 起始寄存器地址
   * @param data 待写入数据
   * @param length 写入长度
   * @return 地址范围有效且写入成功返回 true，否则返回 false
   */
  bool WriteRegister(uint8_t address, const uint8_t* data, size_t length);

  /**
   * @brief 绕过用户寄存器范围校验读取内部寄存器
   * @param address 内部寄存器地址
   * @param value 接收寄存器值
   * @return 读取成功返回 true，否则返回 false
   */
  bool ReadRegisterRaw(uint8_t address, uint8_t& value);

  /**
   * @brief 绕过用户寄存器范围校验写入内部寄存器
   * @param address 内部寄存器地址
   * @param value 写入寄存器值
   * @return 写入成功返回 true，否则返回 false
   */
  bool WriteRegisterRaw(uint8_t address, uint8_t value);

};

}  // namespace cpp_bus_driver
