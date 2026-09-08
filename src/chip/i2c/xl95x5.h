/*
 * @Description: XL95x5 GPIO 扩展芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-08-03 16:11:35
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {

class Xl95x5 final : public I2cChipBase {
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

  explicit Xl95x5(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kPinNotConnected)
      : I2cChipBase(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取 XL95x5 芯片标识。
   * @return 芯片标识；读取失败返回 0xFF。
   */
  uint8_t GetChipId();

  /**
   * @brief 设置引脚或端口模式
   * @param pin 使用Pin::配置，引脚号或Pin::kIoPort0/Pin::kIoPort1
   * @param mode 使用Mode::配置，输入或输出模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGpioMode(Pin pin, Mode mode);

  /**
   * @brief 写入引脚或端口数据
   * @param pin
   * 使用Pin::配置；传入引脚时写单个IO，传入Pin::kIoPort0或Pin::kIoPort1时写整个端口
   * @param value
   * 写单个IO时0为低电平、非0为高电平；写端口时每一位对应一个IO输出电平
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
  // 默认 I2C 总线时钟，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 100000;

  // 芯片标识读取失败时返回的无效值。
  static constexpr uint8_t kInvalidChipId = 0xFF;

  enum class Register {
    kRoChipId = 0x04,
    kRoInputPort0 = 0x00,
    kRoInputPort1 = 0x01,
    kRwOutputPort0 = 0x02,
    kRwOutputPort1 = 0x03,
    kRwPolarityInversionPort0 = 0x04,
    kRwPolarityInversionPort1 = 0x05,
    kRwConfigurationPort0 = 0x06,
    kRwConfigurationPort1 = 0x07,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x20;

  /**
   * @brief 读取寄存器，并记录访问失败信息
   * @param reg 寄存器地址
   * @param data 接收缓冲区
   * @param length 读取字节数
   * @return 读取成功返回true，否则返回false
   */
  bool ReadRegister(uint8_t reg, uint8_t* data, size_t length = 1);

  /**
   * @brief 写入寄存器，并记录访问失败信息
   * @param reg 寄存器地址
   * @param value 待写入数据
   * @return 写入成功返回true，否则返回false
   */
  bool WriteRegister(uint8_t reg, uint8_t value);

  int32_t rst_;
};
}  // namespace cpp_bus_driver
