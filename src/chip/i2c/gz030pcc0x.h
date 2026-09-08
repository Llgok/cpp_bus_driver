/*
 * @Description: GZ030PCC0X 显示面板辅助控制驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-08-03 16:11:12
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {
class Gz030pcc0x final : public I2cChipBase {
 public:
  enum class DataFormat {
    kRgb888 = 0B00000011,
    kInternalTestMode = 0B00000101,  // 内部测试图模式
  };

  enum class InternalTestMode {
    kRegisterControlRgb = 0B00000000,
    kPureWhiteField = 0B00100000,
    kPureRedField = 0B01000000,
    kPureGreenField = 0B01100000,
    kPureBlueField = 0B10000000,
    kGrayscaleImage = 0B10100000,
    kColorBar = 0B11000000,
    kCheckerboard = 0B11100000,
  };

  enum class ShowDirection {
    kNormal = 0B00000000,
    kHorizontalMirror = 0B00000001,          // 水平镜像
    kVerticalMirror = 0B00000010,            // 垂直镜像
    kHorizontalVerticalMirror = 0B00000011,  // 水平垂直镜像
  };

  explicit Gz030pcc0x(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kPinNotConnected)
      : I2cChipBase(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 获取温度
   * @return 以°C为单位
   */
  float GetTemperatureCelsius();

  /**
   * @brief 设置数据模式
   * @param format 数据格式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDataFormat(DataFormat format);

  /**
   * @brief 内部测试模式
   * @param mode 内部测试模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetInternalTestMode(InternalTestMode mode);

  /**
   * @brief 设置显示方向
   * @param direction 显示方向
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetShowDirection(ShowDirection direction);

  /**
   * @brief 设置亮度
   * @param value 值范围：0~255
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetBrightness(uint8_t value);

 private:
  // 默认 I2C 总线时钟，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 100000;

  enum class Register {
    kRwInternalTestModeInputDataFormat = 0x0001,
    kRwHorizontalVerticalMirror = 0x02,
    kRwDisplayBrightness = 0x5800,
    kRoTemperatureReading = 0x3001,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x28;
  static constexpr uint16_t kInitSequence[] = {
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x08,

      // 将 MIPI 总线的数据通道数设置为 4。
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x00,

      // 使用 2 条数据通道时，可将寄存器 0x6800 设置为 0x03。

      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x01,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x5F00, 0x22,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x9F00, 0x06,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6801, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6802, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6803, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x70,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x10,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x07,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6801, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6802, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6803, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x70,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x10,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x03,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x0F,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6801, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6802, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6803, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x70,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x14,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x03,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x02,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6801, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6802, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6803, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x70,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x04,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x01,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6801, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6802, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6803, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x70,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x04,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x04,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6801, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6802, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6803, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x70,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x08,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x04,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x11,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6801, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6802, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6803, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x70,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x04,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6801, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6802, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6803, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x70,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x04,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x01,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6801, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6802, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6803, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x70,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x7D02, 0xC0,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x7E03, 0x01,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6F00, 0x30,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x7402, 0x0D,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x9F01, 0x10};

  /**
   * @brief 读取寄存器，并记录访问失败信息
   * @param reg 寄存器地址
   * @param data 接收缓冲区
   * @param length 读取字节数
   * @return 读取成功返回true，否则返回false
   */
  bool ReadRegister(uint16_t reg, uint8_t* data, size_t length = 1);

  /**
   * @brief 写入寄存器，并记录访问失败信息
   * @param reg 寄存器地址
   * @param value 待写入数据
   * @return 写入成功返回true，否则返回false
   */
  bool WriteRegister(uint16_t reg, uint8_t value);

  int32_t rst_;
};
}  // namespace cpp_bus_driver
