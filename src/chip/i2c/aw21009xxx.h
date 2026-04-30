/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:42:54
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Aw21009xxx final : public ChipI2cGuide {
 public:
  enum class LedChannel {
    kLed1 = 0,
    kLed2,
    kLed3,
    kLed4,
    kLed5,
    kLed6,
    kLed7,
    kLed8,
    kLed9,

    kAll,
  };

  explicit Aw21009xxx(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;

  uint8_t GetDeviceId();

  /**
   * @brief 设置自动省电模式
   * @param enable [true]：开启自动省电模式  [false]：关闭自动省电模式
   * @return
   * @Date 2025-09-24 11:24:55
   */
  bool SetAutoPowerSave(bool enable);

  /**
   * @brief 设置芯片使能
   * @param enable [true]：开启芯片  [false]：关闭芯片
   * @return
   * @Date 2025-09-24 11:38:38
   */
  bool SetChipEnable(bool enable);

  /**
   * @brief 设置亮度
   * @param channel 使用Led_Channel::配置
   * @param value 值范围：0~4095
   * @return
   * @Date 2025-09-24 11:39:18
   */
  bool SetBrightness(LedChannel channel, uint16_t value);

  /**
   * @brief 设置亮度
   * @param channel 使用Led_Channel::配置
   * @param value 值范围：0~255
   * @return
   * @Date 2025-09-24 11:39:18
   */
  bool SetCurrentLimit(LedChannel channel, uint8_t value);

  /**
   * @brief 设置全局电流限制
   * @param value 值范围：0~255
   * @return
   * @Date 2025-09-24 13:37:12
   */
  bool SetGlobalCurrentLimit(uint8_t value);

 private:
  enum class Cmd {
    kRoDeviceId = 0x70,

    kRwGlobalControlRegister = 0x20,
    kRwBrightnessControlRegisterStart,

    kWoUpdateRegister = 0x45,
    kRwScalingRegisterStart,

    kRwGlobalCurrentControlRegister = 0x58,

    kRwResetRegister = 0x70,

  };

  static constexpr uint8_t kDeviceI2cAddress1 = 0x20;
  static constexpr uint8_t kDeviceI2cAddress2 = 0x21;
  static constexpr uint8_t kDeviceI2cAddress3 = 0x24;
  static constexpr uint8_t kDeviceI2cAddress4 = 0x25;
  static constexpr uint8_t kDeviceI2cAddressDefault = kDeviceI2cAddress1;
  static constexpr uint8_t kDeviceId = 0x12;
  static constexpr uint8_t kInitSequence[] = {

      // 复位
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwResetRegister), 0,

      static_cast<uint8_t>(InitSequenceFormat::kDelayMs), 10,

      // 启动自动省电模式，12位pwm消抖模式，使能芯片
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwGlobalControlRegister), 0B10000111,

      // 设置全局最大电流为最大
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwGlobalCurrentControlRegister), 0B11111111,

      // 设置所有通道的电流限制为最大
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwScalingRegisterStart), 0B11111111,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) + 1, 0B11111111,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) + 2, 0B11111111,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) + 3, 0B11111111,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) + 4, 0B11111111,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) + 5, 0B11111111,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) + 6, 0B11111111,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) + 7, 0B11111111,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwScalingRegisterStart) + 8, 0B11111111,

      // 更新寄存器
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kWoUpdateRegister), 0};

  int32_t rst_;
};
}  // namespace cpp_bus_driver
