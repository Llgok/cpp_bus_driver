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

  explicit Xl95x5(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kDefaultValue)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit(bool delete_bus = true) override;

  uint8_t GetDeviceId();

  /**
   * @brief 设置引脚或端口模式
   * @param pin 使用Pin::配置，引脚号或Pin::kIoPort0/Pin::kIoPort1
   * @param mode 使用Mode::配置，输入或输出模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGpioMode(Pin pin, Mode mode);

  /**
   * @brief 写入引脚或端口数据
   * @param pin 使用Pin::配置；传入引脚时写单个IO，传入Pin::kIoPort0或Pin::kIoPort1时写整个端口
   * @param value 写单个IO时0为低电平、非0为高电平；写端口时每一位对应一个IO输出电平
   * @return 写入成功返回 true，失败返回 false
   */
  bool GpioWrite(Pin pin, uint8_t value);

  /**
   * @brief 读取引脚或端口数据
   * @param pin 使用Pin::配置，引脚号或Pin::kIoPort0/Pin::kIoPort1
   * @return 读取引脚返回0或1，读取端口返回8位端口数据，失败返回0xFF
   */
  uint8_t GpioRead(Pin pin);

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
