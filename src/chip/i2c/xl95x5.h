/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:44:44
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
      int32_t rst = kDefaultValue)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit(bool delete_bus = true) override;

  uint8_t GetDeviceId();

  /**
   * @brief 设置引脚模式
   * @param pin 使用Pin::配置，引脚号
   * @param mode 使用Mode::配置，模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGpioMode(Pin pin, Mode mode);

  /**
   * @brief 引脚写数据
   * @param pin 使用Pin::配置，引脚号
   * @param value [0]：低电平，[1]：高电平
   * @return 成功返回 true，失败返回 false
   */
  bool GpioWrite(Pin pin, Value value);

  /**
   * @brief 引脚读数据
   * @param pin 使用Pin::配置，引脚号
   * @return [0]：低电平，[1]：高电平
   */
  bool GpioRead(Pin pin);

  /**
   * @brief 清除中断请求
   * @return 操作成功返回 true，失败返回 false
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
