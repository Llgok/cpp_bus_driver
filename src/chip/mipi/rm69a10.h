
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-17 13:37:59
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Rm69a10 final : public ChipMipiGuide {
 public:
  explicit Rm69a10(std::shared_ptr<BusMipiGuide> bus,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipMipiGuide(bus, InitSequenceFormat::kWriteC8D8), rst_(rst) {}

  bool Init(float freq_mhz = CPP_BUS_DRIVER_DEFAULT_VALUE,
      float lane_bit_rate_mbps = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  uint8_t GetDeviceId();

  /**
   * @brief 设置睡眠
   * @param enable [true]：进入睡眠 [false]：退出睡眠
   * @return
   * @Date 2026-01-24 10:37:09
   */
  bool SetSleep(bool enable);

  /**
   * @brief 设置屏幕关闭
   * @param enable [true]：关闭屏幕 [false]：开启屏幕
   * @return
   * @Date 2026-01-24 10:37:09
   */
  bool SetScreenOff(bool enable);

  /**
   * @brief 设置颜色反转
   * @param enable [true]：开启颜色反转 [false]：关闭颜色反转
   * @return
   * @Date 2026-01-24 10:37:09
   */
  bool SetInversion(bool enable);

  /**
   * @brief 设置屏幕亮度
   * @param brightness 亮度值 [0-255]，0最暗，255最亮
   * @return
   * @Date 2026-01-24 10:37:09
   */
  bool SetBrightness(uint8_t brightness);

  /**
   * @brief 发送色彩流以坐标的形式
   * @param x_start x坐标开始点
   * @param x_end x坐标结束点
   * @param y_start y坐标开始点
   * @param y_end x坐标结束点
   * @param *data 颜色数据
   * @return
   * @Date 2026-01-24 17:06:45
   */
  bool SendColorStreamCoordinate(uint16_t x_start, uint16_t x_end,
      uint16_t y_start, uint16_t y_end, const void* data);

 private:
  enum class Cmd {
    kRoDeviceId = 0xA1,

    kWoSlpin = 0x10,
    kWoSlpout,

    kWoInvoff = 0x20,
    kWoInvon,

    kWoWrdisbv = 0x51,

    kWoDispoff = 0x28,
    kWoDispon,
  };

  static constexpr uint8_t kDeviceId = 0x01;
  static constexpr uint8_t kInitSequence[] = {
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8), 0xFE, 0xFD,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8), 0x80, 0xFC,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8), 0xFE, 0x00,

      static_cast<uint8_t>(InitSequenceFormat::kWriteC8ByteData), 0x2A, 4, 0x00,
      0x00, 0x02, 0x37,

      static_cast<uint8_t>(InitSequenceFormat::kWriteC8ByteData), 0x2B, 4, 0x00,
      0x00, 0x04, 0xCF,

      static_cast<uint8_t>(InitSequenceFormat::kWriteC8ByteData), 0x31, 4, 0x00,
      0x03, 0x02, 0x34,

      static_cast<uint8_t>(InitSequenceFormat::kWriteC8ByteData), 0x30, 4, 0x00,
      0x00, 0x04, 0xCF,

      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8), 0x12, 0x00,

      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8), 0x35, 0x00,

      // 设置屏幕亮度为0
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8), 0x51, 0x00,

      static_cast<uint8_t>(InitSequenceFormat::kWriteC8), 0x11,
      static_cast<uint8_t>(InitSequenceFormat::kDelayMs), 120,
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8), 0x29};

  int32_t rst_;
  uint8_t madctl_data_ = 0;
};
};  // namespace cpp_bus_driver