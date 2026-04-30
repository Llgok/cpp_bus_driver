/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:43:21
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Gz030pcc0x final : public ChipI2cGuide {
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

  explicit Gz030pcc0x(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;

  /**
   * @brief 获取温度
   * @return 以°C为单位
   * @Date 2025-08-15 11:41:42
   */
  float GetTemperatureCelsius();

  /**
   * @brief 设置数据模式
   * @param format 使用Data_Format::配置
   * @return
   * @Date 2025-08-15 13:50:37
   */
  bool SetDataFormat(DataFormat format);

  /**
   * @brief 内部测试模式
   * @param mode 使用Internal_Test_Mode::配置
   * @return
   * @Date 2025-08-15 14:05:28
   */
  bool SetInternalTestMode(InternalTestMode mode);

  /**
   * @brief 设置显示方向
   * @param direction 使用Show_Direction::配置
   * @return
   * @Date 2025-09-18 10:52:45
   */
  bool SetShowDirection(ShowDirection direction);

  /**
   * @brief 设置亮度
   * @param value 值范围：0~255
   * @return
   * @Date 2025-09-18 11:05:59
   */
  bool SetBrightness(uint8_t value);

 private:
  enum class Cmd {
    kRwInternalTestModeInputDataFormat = 0x0001,
    kRwHorizontalVerticalMirror,

    kRwDisplayBrightness = 0x5800,

    kRoTemperatureReading = 0x3001,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x28;
  static constexpr uint16_t kInitSequence[] = {
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6C00, 0x00,
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6900, 0x08,

      // Mipi 总线的 lane 数量设置为 4 lane
      static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6901, 0x00,

      // Mipi 总线的 lane 数量设置为 2 lane
      // static_cast<uint16_t>(InitSequenceFormat::kWriteC16D8), 0x6800, 0x03,

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

  int32_t rst_;
};
}  // namespace cpp_bus_driver
