
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-21 11:39:05
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Xl95x5 final : public ChipI2cGuide {
 public:
  enum class Pin {
    kIo0 = 0,
    kIo1,
    kIo2,
    kIo3,
    kIo4,
    kIo5,
    kIo6,
    kIo7,

    kIo10 = 10,
    kIo11,
    kIo12,
    kIo13,
    kIo14,
    kIo15,
    kIo16,
    kIo17,

    kIoPort0,
    kIoPort1,
  };

  enum class Mode {
    kOutput,
    kInput,
  };

  enum class Value {
    kLow = 0,
    kHigh,
  };

  explicit Xl95x5(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  uint8_t GetDeviceId();

  /**
   * @brief 设置引脚模式
   * @param pin 使用Pin::配置，引脚号
   * @param mode 使用Mode::配置，模式
   * @return
   * @Date 2025-03-11 10:44:12
   */
  bool SetPinMode(Pin pin, Mode mode);

  /**
   * @brief 引脚写数据
   * @param pin 使用Pin::配置，引脚号
   * @param value [0]：低电平，[1]：高电平
   * @return
   * @Date 2025-03-11 10:44:53
   */
  bool PinWrite(Pin pin, Value value);

  /**
   * @brief 引脚读数据
   * @param pin 使用Pin::配置，引脚号
   * @return [0]：低电平，[1]：高电平
   * @Date 2025-03-11 10:46:16
   */
  bool PinRead(Pin pin);

  /**
   * @brief 清除中断请求
   * @return
   * @Date 2025-03-11 10:48:11
   */
  bool ClearIrqFlag();

 private:
  enum class Cmd {
    kRoDeviceId = 0x04,

    kRoInputPort0 = 0x00,
    kRoInputPort1,
    kRwOutputPort0,
    kRwOutputPort1,
    kRwPolarityInversionPort0,
    kRwPolarityInversionPort1,
    kRwConfigurationPort0,
    kRwConfigurationPort1,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x20;

  int32_t rst_;
};
}  // namespace cpp_bus_driver