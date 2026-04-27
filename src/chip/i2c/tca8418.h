
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-17 14:03:15
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Tca8418 final : public ChipI2cGuide {
 public:
  // 需要清除的中断请求参数，设置1代表清除
  enum class IrqFlag {
    kAll = 0B00011111,  // 全部中断
    kCtrlAltDelKeySequence = 0B00010000,
    kFifoOverflow = 0B00001000,
    kKeypadLock = 0B00000100,
    GPIO_INTERRUPT = 0B00000010,
    kKeyEvents = 0B00000001,
  };

  // 中断掩码，设置中断开关
  enum class IrqMask {
    kAll = 0B00001111,  // 全部中断
    kFifoOverflow = 0B00001000,
    kKeypadLock = 0B00000100,
    GPIO_INTERRUPT = 0B00000010,
    kKeyEvents = 0B00000001,
  };

  // 事件类型
  enum class EventType {
    kKeypad,  // 0~80为触发键盘事件
    kGpio,    // 97~114为触发GPIO事件
  };

  struct TouchInfo {
    uint8_t num = -1;  // 序号
    EventType event_type = EventType::kKeypad;
    bool press_flag = false;  // 按压标志
  };

  struct TouchPoint {
    uint8_t finger_count = -1;  // 触摸手指总数

    std::vector<struct TouchInfo> info;
  };

  struct IrqStatus  // 中断状态
  {
    bool ctrl_alt_del_key_sequence_flag =
        false;  // ctrl+alt+del 这一系统级组合键的同时按下动作标志
    bool fifo_overflow_flag = false;   // fifo溢出中断标志
    bool keypad_lock_flag = false;     // 键盘锁定中断标志
    bool gpio_interrupt_flag = false;  // gpio中断标志
    bool key_events_flag = false;      // 按键事件标志
  };

  struct TouchPosition {
    uint8_t x = -1;  // x 坐标
    uint8_t y = -1;  // y 坐标
  };

  explicit Tca8418(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  /**
   * @brief 设置键盘扫描模式开窗大小
   * @param x 开窗点x坐标，值范围（0~9）
   * @param y 开窗点y坐标，值范围（0~7）
   * @param w 开窗长度，值范围（0~9）
   * @param h 开窗高度，值范围（0~7）
   * @return
   * @Date 2025-07-30 13:42:08
   */
  bool SetKeypadScanWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

  /**
   * @brief 获取触摸总数
   * @return
   * @Date 2025-07-30 15:23:03
   */
  uint8_t GetFingerCount();

  /**
   * @brief 获取多个触控的触摸点信息
   * @param &tp 使用结构体Touch_Point::配置触摸点结构体
   * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
   * @Date 2025-07-30 16:16:10
   */
  bool GetMultipleTouchPoint(TouchPoint& tp);

  /**
   * @brief 获取中断标志
   * @return
   * @Date 2025-07-30 16:39:43
   */
  uint8_t GetIrqFlag();

  /**
   * @brief 中断解析，详细请参考TCA8418手册 8.6.2.2 Interrupt Status Register,
   * INT_STAT (Address 0x02)
   * @param irq_flag 解析状态语句，由get_irq_flag()函数获取
   * @param &status 使用Irq_Status结构体配置，相应位自动置位
   * @return
   * @Date 2025-07-30 16:39:56
   */
  bool ParseIrqStatus(uint8_t irq_flag, IrqStatus& status);

  /**
   * @brief 清除中断标志位
   * @param flag 使用Irq_Flag::配置，需要清除的标志，设置1为清除标志
   * @return
   * @Date 2025-07-30 16:55:59
   */
  bool ClearIrqFlag(IrqFlag flag);

  /**
   * @brief 获取gpio中断同时清除gpio中断标志
   * @return
   * @Date 2025-07-31 09:05:05
   */
  uint32_t GetClearGpioIrqFlag();

  /**
   * @brief 设置中断引脚模式
   * @param mode 由Irq_Mask::配置，选择需要开启的中断引脚位
   * @return
   * @Date 2025-07-31 10:16:03
   */
  bool SetIrqPinMode(IrqMask mode);

  /**
   * @brief 用于解码触摸号数为x、y坐标
   * @param num 结构体Touch_Info中的num值，解码前需要先获取该值
   * @param &position 使用Touch_Position::配置，解码后的坐标信息
   * @return
   * @Date 2025-07-31 13:39:08
   */
  bool ParseTouchNum(uint8_t num, TouchPosition& position);

 private:
  enum class Cmd {
    kRwConfiguration = 0x01,
    kRwInterruptStatus,
    kRwKeyLockAndEventCounter,
    kRoKeyEvent,

    kRoGpioInterruptStatusStart = 0x11,

    kWoGpioIntEn1 = 0x1A,  // GPIO中断使能1
    kWoGpioIntEn2,         // GPIO中断使能2
    kWoGpioIntEn3,         // GPIO中断使能3
    kWoKpGpio1,            // 按键或GPIO模式选择1
    kWoKpGpio2,            // 按键或GPIO模式选择2
    kWoKpGpio3,            // 按键或GPIO模式选择3

    kWoGpiEm1 = 0x20,  // GPI事件模式1
    kWoGpiEm2,         // GPI事件模式2
    kWoGpiEm3,         // GPI事件模式3
    kWoGpioDir1,       // GPIO数据方向1
    kWoGpioDir2,       // GPIO数据方向2
    kWoGpioDir3,       // GPIO数据方向3
    kWoGpioIntLvl1,    // GPIO边沿/电平检测1
    kWoGpioIntLvl2,    // GPIO边沿/电平检测2
    kWoGpioIntLvl3     // GPIO边沿/电平检测3

  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x34;
  static constexpr uint8_t kMaxWidthSize = 10;
  static constexpr uint8_t kMaxHeightSize = 8;
  static constexpr uint8_t kInitSequence[] = {

      // 开启寄存器读写自动递增
      // 开启fifo溢出自动栈队列循环
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwConfiguration), 0B10100000,

      // 设置默认全部引脚为输入
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      // static_cast<uint8_t>(Cmd::kWoGpioDir1), 0x00,
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      // static_cast<uint8_t>(Cmd::kWoGpioDir2), 0x00,
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      // static_cast<uint8_t>(Cmd::kWoGpioDir3), 0x00,

      // 添加全部gpio事件到fifo
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kWoGpiEm1), 0xFF,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kWoGpiEm2), 0xFF,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kWoGpiEm3), 0xFF,

      // 设置全部引脚中断为下降沿中断
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      // static_cast<uint8_t>(Cmd::kWoGpioIntLvl1), 0x00,
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      // static_cast<uint8_t>(Cmd::kWoGpioIntLvl2), 0x00,
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      // static_cast<uint8_t>(Cmd::kWoGpioIntLvl3), 0x00,

      // 添加全部引脚到中断
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kWoGpioIntEn1), 0xFF,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kWoGpioIntEn2), 0xFF,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kWoGpioIntEn3), 0xFF};

  int32_t rst_;
};
}  // namespace cpp_bus_driver