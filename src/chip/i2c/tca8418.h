/*
 * @Description: TCA8418 键盘扫描与 GPIO 扩展芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-06-01 17:30:00
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Tca8418 final : public ChipI2cGuide {
 public:
  // TCA8418 中断状态位，写 1 清除对应中断。
  enum class IrqFlag : uint8_t {
    kNone = 0x00,
    kKeyEvents = 0x01,
    kGpioInterrupt = 0x02,
    kKeypadLock = 0x04,
    kFifoOverflow = 0x08,
    kCtrlAltDelKeySequence = 0x10,
    kAll = 0x1F,
  };

  // TCA8418 主中断输出使能位。
  enum class IrqMask : uint8_t {
    kNone = 0x00,
    kKeyEvents = 0x01,
    kGpioInterrupt = 0x02,
    kKeypadLock = 0x04,
    kFifoOverflow = 0x08,
    kAll = 0x0F,
  };

  // FIFO 事件类型。
  enum class EventType : uint8_t {
    kKeypad = 0,
    kGpio,
    kUnknown,
  };

  // TCA8418 可复用 GPIO 引脚。
  enum class Gpio : uint8_t {
    kRow0 = 0,
    kRow1,
    kRow2,
    kRow3,
    kRow4,
    kRow5,
    kRow6,
    kRow7,
    kCol0,
    kCol1,
    kCol2,
    kCol3,
    kCol4,
    kCol5,
    kCol6,
    kCol7,
    kCol8,
    kCol9,
  };

  // GPIO 输入输出方向。
  enum class GpioMode : uint8_t {
    kInput = 0,
    kOutput,
  };

  // GPIO 中断触发极性。
  enum class GpioInterruptTrigger : uint8_t {
    kFallingOrLow = 0,
    kRisingOrHigh,
  };

  // TCA8418 配置寄存器内容。
  struct Configuration {
    bool auto_increment = true;
    bool gpi_events_disabled_when_locked = false;
    bool overflow_mode = true;
    bool interrupt_pulse_mode = false;
    bool overflow_interrupt = false;
    bool keypad_lock_interrupt = false;
    bool gpio_interrupt = false;
    bool key_events_interrupt = false;
  };

  // 单个 FIFO 事件。
  struct TouchInfo {
    uint8_t num = 0;
    EventType event_type = EventType::kUnknown;
    bool press_flag = false;
  };

  // FIFO 中读取到的一组事件。
  struct TouchPoint {
    uint8_t finger_count = 0;
    std::vector<TouchInfo> info;
  };

  // 已解析的中断状态。
  struct IrqStatus {
    bool ctrl_alt_del_key_sequence_flag = false;
    bool fifo_overflow_flag = false;
    bool keypad_lock_flag = false;
    bool gpio_interrupt_flag = false;
    bool key_events_flag = false;
  };

  // 键盘矩阵中的坐标。
  struct TouchPosition {
    uint8_t x = 0;
    uint8_t y = 0;
  };

  // 键盘锁状态和 FIFO 事件计数。
  struct KeyLockInfo {
    bool key_lock_enabled = false;
    bool lock1_status = false;
    bool lock2_status = false;
    bool locked = false;
    uint8_t event_count = 0;
  };

  explicit Tca8418(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kDefaultValue)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 将 GPIO 枚举转换为 18 位掩码，bit0~bit7 为 ROW0~ROW7，
   * bit8~bit17 为 COL0~COL9。
   * @param gpio GPIO 引脚。
   * @return GPIO 掩码。
   */
  static constexpr uint32_t GpioMask(Gpio gpio) {
    return static_cast<uint32_t>(1UL) << static_cast<uint8_t>(gpio);
  }

  /**
   * @brief 获取全部 TCA8418 GPIO 的掩码。
   * @return 18 位 GPIO 掩码。
   */
  static constexpr uint32_t AllGpioMask() { return kAllGpioMask; }

  /**
   * @brief 设置完整配置寄存器。
   * @param config 配置寄存器内容。
   * @return 设置成功返回 true。
   */
  bool SetConfiguration(const Configuration& config);

  /**
   * @brief 读取完整配置寄存器。
   * @param config 输出配置寄存器内容。
   * @return 读取成功返回 true。
   */
  bool GetConfiguration(Configuration* config);

  /**
   * @brief 设置寄存器读写自动地址递增功能。
   * @param enable true 为启用，false 为关闭。
   * @return 设置成功返回 true。
   */
  bool SetAutoIncrement(bool enable);

  /**
   * @brief 设置键盘锁定时是否忽略 GPI FIFO 事件。
   * @param disable true 为锁定时忽略，false 为锁定时继续记录。
   * @return 设置成功返回 true。
   */
  bool SetGpiEventsDisabledWhenLocked(bool disable);

  /**
   * @brief 设置 FIFO 溢出模式。
   * @param enable true 为新事件顶出旧事件，false 为丢弃新溢出事件。
   * @return 设置成功返回 true。
   */
  bool SetFifoOverflowMode(bool enable);

  /**
   * @brief 设置中断清除时是否产生 50us 释放脉冲。
   * @param enable true 为启用脉冲模式。
   * @return 设置成功返回 true。
   */
  bool SetInterruptPulseMode(bool enable);

  /**
   * @brief 设置主中断输出使能。
   * @param mask 使用 IrqMask 选择要启用的中断。
   * @return 设置成功返回 true。
   */
  bool SetInterruptEnable(IrqMask mask);

  /**
   * @brief 设置主中断输出使能，可组合多个 IrqMask 位。
   * @param mask 可组合的 IrqMask 位掩码。
   * @return 设置成功返回 true。
   */
  bool SetInterruptEnable(uint8_t mask);

  /**
   * @brief 设置键盘扫描窗口。
   * @param x 起始列，范围 0~9。
   * @param y 起始行，范围 0~7。
   * @param w 扫描列数。
   * @param h 扫描行数。
   * @return 设置成功返回 true。
   */
  bool SetKeypadScanWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

  /**
   * @brief 直接设置哪些 ROW/COL 引脚加入键盘矩阵。
   * @param keypad_mask 18 位 GPIO 掩码。
   * @return 设置成功返回 true。
   */
  bool SetKeypadPins(uint32_t keypad_mask);

  /**
   * @brief 读取哪些 ROW/COL 引脚已加入键盘矩阵。
   * @param keypad_mask 输出 18 位 GPIO 掩码。
   * @return 读取成功返回 true。
   */
  bool GetKeypadPins(uint32_t* keypad_mask);

  /**
   * @brief 获取 FIFO 中待读取事件数量。
   * @return 成功返回 0~10，失败返回 0xFF。
   */
  uint8_t GetFingerCount();

  /**
   * @brief 读取一个 FIFO 事件。
   * @param event 输出事件。
   * @return 读取到有效事件返回 true。
   */
  bool ReadKeyEvent(TouchInfo* event);

  /**
   * @brief 读取 FIFO 中所有当前事件。
   * @param tp 输出事件列表。
   * @return 读取到至少一个事件返回 true。
   */
  bool GetMultipleTouchPoint(TouchPoint& tp);

  /**
   * @brief 读取中断状态寄存器。
   * @return 成功返回中断状态位，失败返回 0xFF。
   */
  uint8_t GetIrqFlag();

  /**
   * @brief 解析中断状态。
   * @param irq_flag GetIrqFlag() 读取到的状态。
   * @param status 输出解析结果。
   * @return 解析成功返回 true。
   */
  bool ParseIrqStatus(uint8_t irq_flag, IrqStatus& status);

  /**
   * @brief 清除指定中断状态位。
   * @param flag 使用 IrqFlag 选择要清除的中断。
   * @return 清除成功返回 true。
   */
  bool ClearIrqFlag(IrqFlag flag);

  /**
   * @brief 清除一个或多个中断状态位。
   * @param flags 可组合的 IrqFlag 位掩码。
   * @return 清除成功返回 true。
   */
  bool ClearIrqFlag(uint8_t flags);

  /**
   * @brief 清空 FIFO 中所有按键/GPI 事件。
   * @return 清空成功返回 true。
   */
  bool ClearEventFifo();

  /**
   * @brief 读取并通过输出参数返回已清除的 GPIO 中断状态。
   * @param status 输出 18 位 GPIO 中断状态掩码。
   * @return 读取成功返回 true。
   */
  bool GetClearGpioIrqFlag(uint32_t* status);

  /**
   * @brief 读取并直接返回已清除的 GPIO 中断状态。
   * @return 成功返回 18 位 GPIO 中断状态掩码，失败返回 0。
   */
  uint32_t GetClearGpioIrqFlag();

  /**
   * @brief 兼容旧接口，设置主中断输出使能。
   * @param mode 使用 IrqMask 选择要启用的中断。
   * @return 设置成功返回 true。
   */
  bool SetIrqGpioMode(IrqMask mode);

  /**
   * @brief 兼容旧接口，设置主中断输出使能，可组合多个 IrqMask 位。
   * @param mode 可组合的 IrqMask 位掩码。
   * @return 设置成功返回 true。
   */
  bool SetIrqGpioMode(uint8_t mode);

  /**
   * @brief 将键盘事件编号解析为矩阵坐标。
   * @param num FIFO 事件编号，范围 1~80。
   * @param position 输出坐标，x 为列，y 为行。
   * @return 解析成功返回 true。
   */
  bool ParseTouchNum(uint8_t num, TouchPosition& position);

  /**
   * @brief 读取键盘锁和 FIFO 计数寄存器。
   * @param info 输出键盘锁信息。
   * @return 读取成功返回 true。
   */
  bool GetKeyLockInfo(KeyLockInfo* info);

  /**
   * @brief 启用或关闭键盘锁。
   * @param enable true 为锁定键盘，false 为手动解锁。
   * @return 设置成功返回 true。
   */
  bool SetKeypadLock(bool enable);

  /**
   * @brief 设置键盘锁定计时器。
   * @param lock_to_lock_timer_s UNLOCK1 到 UNLOCK2 的超时时间，范围 0~7 秒。
   * @param interrupt_mask_timer_s 中断屏蔽时间，范围 0~31 秒。
   * @return 设置成功返回 true。
   */
  bool SetKeypadLockTimer(
      uint8_t lock_to_lock_timer_s, uint8_t interrupt_mask_timer_s);

  /**
   * @brief 设置键盘解锁按键序列。
   * @param unlock_key_1 第一个解锁按键编号，0 为关闭键盘锁功能。
   * @param unlock_key_2 第二个解锁按键编号，0 为关闭键盘锁功能。
   * @return 设置成功返回 true。
   */
  bool SetUnlockKeys(uint8_t unlock_key_1, uint8_t unlock_key_2);

  /**
   * @brief 设置 GPIO 输入输出方向。
   * @param gpio GPIO 引脚。
   * @param mode 输入或输出。
   * @return 设置成功返回 true。
   */
  bool SetGpioDirection(Gpio gpio, GpioMode mode);

  /**
   * @brief 批量设置 GPIO 输入输出方向。
   * @param gpio_mask 18 位 GPIO 掩码。
   * @param mode 输入或输出。
   * @return 设置成功返回 true。
   */
  bool SetGpioDirectionMask(uint32_t gpio_mask, GpioMode mode);

  /**
   * @brief 读取 GPIO 方向寄存器。
   * @param output_mask 输出方向为输出的 GPIO 掩码。
   * @return 读取成功返回 true。
   */
  bool GetGpioDirectionMask(uint32_t* output_mask);

  /**
   * @brief 设置单个 GPIO 输出电平。
   * @param gpio GPIO 引脚。
   * @param high true 输出高电平，false 输出低电平。
   * @return 设置成功返回 true。
   */
  bool SetGpioOutput(Gpio gpio, bool high);

  /**
   * @brief 批量写入 GPIO 输出寄存器。
   * @param high_mask 输出为高电平的 GPIO 掩码。
   * @return 设置成功返回 true。
   */
  bool SetGpioOutputMask(uint32_t high_mask);

  /**
   * @brief 批量设置部分 GPIO 输出电平。
   * @param gpio_mask 需要修改的 GPIO 掩码。
   * @param high_mask 这些 GPIO 中要输出高电平的掩码。
   * @return 设置成功返回 true。
   */
  bool SetGpioOutputMask(uint32_t gpio_mask, uint32_t high_mask);

  /**
   * @brief 读取 GPIO 输出寄存器。
   * @param high_mask 输出为高电平的 GPIO 掩码。
   * @return 读取成功返回 true。
   */
  bool GetGpioOutputMask(uint32_t* high_mask);

  /**
   * @brief 读取 GPIO 数据状态寄存器。
   * @param status 输出 GPIO 状态掩码。
   * @param read_twice 是否连续读取两次以清除锁存状态。
   * @return 读取成功返回 true。
   */
  bool GetGpioDataStatus(uint32_t* status, bool read_twice = false);

  /**
   * @brief 读取单个 GPIO 当前状态。
   * @param gpio GPIO 引脚。
   * @param high 输出状态。
   * @return 读取成功返回 true。
   */
  bool GetGpioInput(Gpio gpio, bool* high);

  /**
   * @brief 设置单个 GPIO 中断使能。
   * @param gpio GPIO 引脚。
   * @param enable true 为启用中断。
   * @return 设置成功返回 true。
   */
  bool SetGpioInterruptEnable(Gpio gpio, bool enable);

  /**
   * @brief 批量设置 GPIO 中断使能。
   * @param gpio_mask 18 位 GPIO 掩码。
   * @param enable true 为启用中断。
   * @return 设置成功返回 true。
   */
  bool SetGpioInterruptEnableMask(uint32_t gpio_mask, bool enable);

  /**
   * @brief 读取 GPIO 中断使能寄存器。
   * @param enable_mask 输出已启用中断的 GPIO 掩码。
   * @return 读取成功返回 true。
   */
  bool GetGpioInterruptEnableMask(uint32_t* enable_mask);

  /**
   * @brief 设置单个 GPIO 中断触发极性。
   * @param gpio GPIO 引脚。
   * @param trigger 触发极性。
   * @return 设置成功返回 true。
   */
  bool SetGpioInterruptTrigger(Gpio gpio, GpioInterruptTrigger trigger);

  /**
   * @brief 批量设置 GPIO 中断触发极性。
   * @param gpio_mask 18 位 GPIO 掩码。
   * @param trigger 触发极性。
   * @return 设置成功返回 true。
   */
  bool SetGpioInterruptTriggerMask(
      uint32_t gpio_mask, GpioInterruptTrigger trigger);

  /**
   * @brief 读取 GPIO 中断触发极性寄存器。
   * @param rising_high_mask 输出配置为上升沿/高电平触发的 GPIO 掩码。
   * @return 读取成功返回 true。
   */
  bool GetGpioInterruptTriggerMask(uint32_t* rising_high_mask);

  /**
   * @brief 设置单个 GPI 是否进入 FIFO 事件表。
   * @param gpio GPIO 引脚。
   * @param enable true 为加入 FIFO 事件表。
   * @return 设置成功返回 true。
   */
  bool SetGpiEventMode(Gpio gpio, bool enable);

  /**
   * @brief 批量设置 GPI 是否进入 FIFO 事件表。
   * @param gpio_mask 18 位 GPIO 掩码。
   * @param enable true 为加入 FIFO 事件表。
   * @return 设置成功返回 true。
   */
  bool SetGpiEventModeMask(uint32_t gpio_mask, bool enable);

  /**
   * @brief 读取 GPI FIFO 事件模式寄存器。
   * @param enable_mask 输出已加入 FIFO 事件表的 GPIO 掩码。
   * @return 读取成功返回 true。
   */
  bool GetGpiEventModeMask(uint32_t* enable_mask);

  /**
   * @brief 设置单个 GPIO 去抖功能。
   * @param gpio GPIO 引脚。
   * @param enable true 为启用去抖。
   * @return 设置成功返回 true。
   */
  bool SetGpioDebounce(Gpio gpio, bool enable);

  /**
   * @brief 批量设置 GPIO 去抖功能。
   * @param gpio_mask 18 位 GPIO 掩码。
   * @param enable true 为启用去抖。
   * @return 设置成功返回 true。
   */
  bool SetGpioDebounceMask(uint32_t gpio_mask, bool enable);

  /**
   * @brief 读取 GPIO 去抖使能状态。
   * @param enable_mask 输出已启用去抖的 GPIO 掩码。
   * @return 读取成功返回 true。
   */
  bool GetGpioDebounceEnabledMask(uint32_t* enable_mask);

  /**
   * @brief 设置单个 GPIO 内部上拉。
   * @param gpio GPIO 引脚。
   * @param enable true 为启用内部上拉。
   * @return 设置成功返回 true。
   */
  bool SetGpioPullup(Gpio gpio, bool enable);

  /**
   * @brief 批量设置 GPIO 内部上拉。
   * @param gpio_mask 18 位 GPIO 掩码。
   * @param enable true 为启用内部上拉。
   * @return 设置成功返回 true。
   */
  bool SetGpioPullupMask(uint32_t gpio_mask, bool enable);

  /**
   * @brief 读取 GPIO 内部上拉使能状态。
   * @param enable_mask 输出已启用内部上拉的 GPIO 掩码。
   * @return 读取成功返回 true。
   */
  bool GetGpioPullupEnabledMask(uint32_t* enable_mask);

 private:
  // TCA8418 寄存器地址。
  enum class Register : uint8_t {
    kConfiguration = 0x01,
    kInterruptStatus = 0x02,
    kKeyLockAndEventCounter = 0x03,
    kKeyEventA = 0x04,
    kKeypadLockTimer = 0x0E,
    kUnlock1 = 0x0F,
    kUnlock2 = 0x10,
    kGpioInterruptStatus1 = 0x11,
    kGpioDataStatus1 = 0x14,
    kGpioDataOut1 = 0x17,
    kGpioInterruptEnable1 = 0x1A,
    kKeypadGpio1 = 0x1D,
    kGpiEventMode1 = 0x20,
    kGpioDirection1 = 0x23,
    kGpioInterruptLevel1 = 0x26,
    kDebounceDisable1 = 0x29,
    kGpioPullupDisable1 = 0x2C,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x34;
  static constexpr uint8_t kRowCount = 8;
  static constexpr uint8_t kColumnCount = 10;
  static constexpr uint8_t kGpioCount = 18;
  static constexpr uint8_t kFifoDepth = 10;
  static constexpr uint8_t kInvalidU8 = 0xFF;
  static constexpr uint8_t kValidIrqMask = 0x1F;
  static constexpr uint8_t kValidInterruptEnableMask = 0x0F;
  static constexpr uint8_t kKeypadFirstEvent = 1;
  static constexpr uint8_t kKeypadLastEvent = 80;
  static constexpr uint8_t kGpioFirstEvent = 97;
  static constexpr uint8_t kGpioLastEvent = 114;
  static constexpr uint32_t kAllGpioMask = 0x0003FFFF;

  /**
   * @brief 将配置结构体打包为 CONFIG 寄存器值。
   * @param config 配置结构体。
   * @return CONFIG 寄存器值。
   */
  static uint8_t PackConfiguration(const Configuration& config);

  /**
   * @brief 将 CONFIG 寄存器值解析为配置结构体。
   * @param value CONFIG 寄存器值。
   * @return 配置结构体。
   */
  static Configuration UnpackConfiguration(uint8_t value);

  /**
   * @brief 根据 FIFO 事件编号判断事件类型。
   * @param event_num FIFO 事件编号。
   * @return 事件类型。
   */
  static EventType DecodeEventType(uint8_t event_num);

  /**
   * @brief 判断 FIFO 事件编号是否有效。
   * @param event_num FIFO 事件编号。
   * @return 事件编号有效返回 true。
   */
  static bool IsValidFifoEventNumber(uint8_t event_num);

  /**
   * @brief 判断解锁按键编号是否有效。
   * @param key_num 解锁按键编号，0 表示关闭键盘锁功能。
   * @return 解锁按键编号有效返回 true。
   */
  static bool IsValidUnlockKeyNumber(uint8_t key_num);

  /**
   * @brief 判断 GPIO 枚举值是否在 TCA8418 可用范围内。
   * @param gpio GPIO 枚举值。
   * @return GPIO 有效返回 true。
   */
  static bool IsValidGpio(Gpio gpio);

  /**
   * @brief 读取一个 TCA8418 寄存器。
   * @param reg 寄存器地址枚举。
   * @param value 输出寄存器值。
   * @return 读取成功返回 true。
   */
  bool ReadRegister(Register reg, uint8_t* value);

  /**
   * @brief 写入一个 TCA8418 寄存器。
   * @param reg 寄存器地址枚举。
   * @param value 寄存器值。
   * @return 写入成功返回 true。
   */
  bool WriteRegister(Register reg, uint8_t value);

  /**
   * @brief 修改一个 TCA8418 寄存器中的指定 bit。
   * @param reg 寄存器地址枚举。
   * @param mask 需要修改的 bit 掩码。
   * @param value 写入到掩码区域的值。
   * @return 修改成功返回 true。
   */
  bool UpdateRegisterBits(Register reg, uint8_t mask, uint8_t value);

  /**
   * @brief 连续读取 3 个 GPIO 相关寄存器并转换为 18 位掩码。
   * @param start_reg GPIO 寄存器组起始地址枚举。
   * @param value 输出 18 位 GPIO 掩码。
   * @return 读取成功返回 true。
   */
  bool ReadGpioRegisterBlock(Register start_reg, uint32_t* value);

  /**
   * @brief 将 18 位掩码写入连续 3 个 GPIO 相关寄存器。
   * @param start_reg GPIO 寄存器组起始地址枚举。
   * @param value 18 位 GPIO 掩码。
   * @return 写入成功返回 true。
   */
  bool WriteGpioRegisterBlock(Register start_reg, uint32_t value);

  /**
   * @brief 修改连续 3 个 GPIO 相关寄存器中的指定 bit。
   * @param start_reg GPIO 寄存器组起始地址枚举。
   * @param mask 需要修改的 18 位 GPIO 掩码。
   * @param value 写入到掩码区域的 18 位 GPIO 值。
   * @return 修改成功返回 true。
   */
  bool UpdateGpioRegisterBlock(
      Register start_reg, uint32_t mask, uint32_t value);

  int32_t rst_;
};

}  // namespace cpp_bus_driver
