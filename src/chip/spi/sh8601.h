
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-17 14:05:02
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Sh8601 final : public ChipQspiGuide {
 public:
  enum class ColorFormat {
    kRgb565 = 16,
    kRgb666 = 18,
    kRgb888 = 24,
  };

  // 色彩增强
  enum class ColorEnhance {
    kOff = 0x00,
    kLow = 0x04,
    kMedium = 0x06,
    kHigh,
  };

  enum class WriteStreamMode {
    // 单线模式
    kWrite1lanes,
    // 4线模式
    kWrite4lanes,
    // 连续发射单线模式
    kContinuousWrite1lanes,
    // 连续发射4线模式
    kContinuousWrite4lanes,
  };

  explicit Sh8601(std::shared_ptr<BusQspiGuide> bus, uint16_t width,
      uint16_t height, int32_t cs = CPP_BUS_DRIVER_DEFAULT_VALUE,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE, int16_t x_offset = 0,
      int16_t y_offset = 0, ColorFormat color_format = ColorFormat::kRgb565)
      : ChipQspiGuide(bus, cs),
        rst_(rst),
        width_(width),
        height_(height),
        x_offset_(x_offset),
        y_offset_(y_offset),
        color_format_(color_format) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  /**
   * @brief 设置需要渲染的窗口
   * @param x_start x坐标开始点
   * @param x_end x坐标结束点
   * @param y_start y坐标开始点
   * @param y_end x坐标结束点
   * @return
   * @Date 2025-06-30 11:10:12
   */
  bool SetRenderWindow(
      uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end);

  /**
   * @brief 设置写入流的模式
   * @param mode 使用Write_Stream_Mode::配置
   * @return
   * @Date 2025-07-11 09:04:02
   */
  bool SetWriteStreamMode(WriteStreamMode mode);

  /**
   * @brief 发送颜色流
   * @param x 绘制点x坐标
   * @param y 绘制点y坐标
   * @param w 绘制长度
   * @param h 绘制高度
   * @param *data 颜色数据
   * @return
   * @Date 2025-07-03 10:27:58
   */
  bool SendColorStream(
      uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data);

  /**
   * @brief 设置亮度
   * @param value 值范围：0~255
   * @return
   * @Date 2025-07-11 09:42:35
   */
  bool SetBrightness(uint8_t value);

  /**
   * @brief 设置睡眠
   * @param status [true]：进入睡眠 [false]：退出睡眠
   * @return
   * @Date 2025-07-11 11:52:48
   */
  bool SetSleep(bool status);

  /**
   * @brief 设置屏幕关闭
   * @param enable [true]：关闭屏幕 [false]：开启屏幕
   * @return
   * @Date 2025-07-11 11:52:48
   */
  bool SetScreenOff(bool enable);

  /**
   * @brief 设置颜色增强模式
   * @param enable 使用Color_Enhance::配置
   * @return
   * @Date 2025-07-11 13:48:16
   */
  bool SetColorEnhance(ColorEnhance enable);

  /**
   * @brief 设置颜色格式
   * @param format 使用Color_Format::配置
   * @return
   * @Date 2025-07-14 09:25:25
   */
  bool SetColorFormat(ColorFormat format);

 private:
  enum class Cmd {
    // 用于读写寄存器命令
    kWoWriteRegister = 0x02,
    kWoReadRegister,

    // 用于写颜色流命令
    kWoWriteColorStream1lanesCmd = 0x02,
    kWoWriteColorStream4lanesCmd2 = 0x12,
    kWoWriteColorStream4lanesCmd1 = 0x32,

  };

  enum class Reg {
    // 用于写颜色流命令
    // 从指定的像素位置开始写入图像数据，该位置由之前的 kCaset
    // (2Ah)（列地址设置）和 RASET (2Bh)（行地址设置）命令定义
    kWoMemoryStartWrite = 0x002C00,
    // 从上一次写入结束的位置继续写入数据，无需重新指定地址
    kWoMemoryContinuousWrite = 0x003C00,

    kWoColumnAddressSet = 0x002A00,
    kWoPageAddressSet = 0x002B00,
    kWoMemoryWriteStart = 0x002C00,
    kWoWriteDisplayBrightness = 0x005100,

    kWoSleepOut = 0x001100,
    kWoSleepIn = 0x001000,
    kWoDisplayOn = 0x002900,
    kWoDisplayOff = 0x002800,

    kWoInterfacePixelFormat = 0x003A00,

    kWoSetColorEnhance = 0x005800,

  };

  static constexpr uint32_t kInitSequence[] = {
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24),
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      0x001100,

      static_cast<uint8_t>(InitSequenceFormat::kDelayMs),
      120,

      // normal display mode on
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24),
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      0x001300,

      // RGB
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      0x003600,
      0x00,

      // BGR
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      // static_cast<uint8_t>(Cmd::kWoWriteRegister), 0x003600, 0x08,

      // interface pixel format 16bit/pixel
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      0x003A00,
      0x55,

      // interface pixel format 18bit/pixel
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      // static_cast<uint8_t>(Cmd::kWoWriteRegister), 0x003A00, 0x66,

      // interface pixel format 24bit/pixel
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      // static_cast<uint8_t>(Cmd::kWoWriteRegister), 0x003A00, 0x77,

      // brightness control on and display dimming on
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      0x005300,
      0x28,

      // write display brightness value in hbm mode
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      0x006300,
      0xFF,

      // brightness adjustment
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      0x005100,
      0x00,

      // sunlight readability enhancement off
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      0x005800,
      0x00,

      // sunlight readability enhancement low
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      // static_cast<uint8_t>(Cmd::kWoWriteRegister), 0x005800, 0x04,

      // sunlight readability enhancement medium
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      // static_cast<uint8_t>(Cmd::kWoWriteRegister), 0x005800, 0x05,

      // sunlight readability enhancement high
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      // static_cast<uint8_t>(Cmd::kWoWriteRegister), 0x005800, 0x06,

      // display on
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24),
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      0x002900,

      static_cast<uint8_t>(InitSequenceFormat::kDelayMs),
      10,

      // brightness adjustment
      // static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      // static_cast<uint8_t>(Cmd::kWoWriteRegister), 0x005100, 0xFF,

  };

  int32_t rst_;
  uint16_t width_, height_;
  int16_t x_offset_, y_offset_;
  ColorFormat color_format_;
};
};  // namespace cpp_bus_driver