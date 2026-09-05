/*
 * @Description: CO5300 QSPI 显示控制器驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-09-05 14:57:09
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {
class Co5300 final : public QspiChipBase {
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
    kMedium,
    kHigh,
  };

  enum class WriteStreamMode {
    // 单线模式
    kWrite1Lane,
    // 4线模式
    kWrite4Lanes,
    // 连续发射单线模式
    kContinuousWrite1Lane,
    // 连续发射4线模式
    kContinuousWrite4Lanes,

  };

  explicit Co5300(std::shared_ptr<QspiBusBase> bus, uint16_t width,
      uint16_t height, int32_t cs = kPinNotConnected,
      int32_t rst = kPinNotConnected, int16_t x_offset = 0,
      int16_t y_offset = 0, ColorFormat color_format = ColorFormat::kRgb565)
      : QspiChipBase(bus, cs),
        rst_(rst),
        width_(width),
        height_(height),
        x_offset_(x_offset),
        y_offset_(y_offset),
        color_format_(color_format) {}

  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;
  bool Deinit() override;

  /**
   * @brief 设置需要渲染的窗口
   * @param x_start x坐标开始点
   * @param y_start y坐标开始点
   * @param x_end x坐标结束点
   * @param y_end y坐标结束点
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRenderWindow(int x_start, int y_start, int x_end, int y_end);

  /**
   * @brief 设置写入流的模式
   * @param mode 写入流模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetWriteStreamMode(WriteStreamMode mode);

  /**
   * @brief 发送颜色流
   * @param x 绘制点x坐标
   * @param y 绘制点y坐标
   * @param w 绘制长度
   * @param h 绘制高度
   * @param data 像素数据指针
   * @return 操作成功返回 true，失败返回 false
   */
  bool SendColorStream(
      uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data);

  /**
   * @brief 设置亮度
   * @param value 值范围：0~255
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetBrightness(uint8_t value);

  /**
   * @brief 设置睡眠
   * @param enable [true]：进入睡眠 [false]：退出睡眠
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetSleep(bool enable);

  /**
   * @brief 设置屏幕关闭
   * @param enable [true]：关闭屏幕 [false]：开启屏幕
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetScreenOff(bool enable);

  /**
   * @brief 设置颜色增强模式
   * @param mode 色彩增强档位
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetColorEnhance(ColorEnhance mode);

  /**
   * @brief 设置颜色格式
   * @param format 像素颜色格式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetColorFormat(ColorFormat format);

 private:
  // 默认 QSPI 总线时钟，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 10000000;

  // QSPI 寄存器事务操作码。
  enum class RegisterOpcode {
    kWrite = 0x02,
    kRead,
  };

  // QSPI 颜色流事务操作码。
  enum class ColorStreamOpcode {
    kOneLane = 0x02,
    kFourLaneCommand2 = 0x12,
    kFourLaneCommand1 = 0x32,
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
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x001100,

      static_cast<uint8_t>(InitSequenceFormat::kDelayMs),
      120,

      // 页面切换
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x00FE00,
      0x00,

      // SPI 模式
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x00C400,
      0x80,

      // RGB 色序
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x003600,
      0x00,

      // BGR 色序可将寄存器 0x003600 设置为 0x08。

      // 接口像素格式：16 位/像素
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x003A00,
      0x55,

      // 18 位像素格式可将寄存器 0x003A00 设置为 0x66。

      // 24 位像素格式可将寄存器 0x003A00 设置为 0x77。

      // 写入显示控制 1
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x005300,
      0x20,

      // 写入 HBM 模式下的显示亮度
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x006300,
      0xFF,

      // 亮度调节
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x005100,
      0x00,

      // 关闭阳光下可读性增强
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8),
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x005800,
      0x00,

      // 阳光下可读性增强的低、中、高档分别为 0x04、0x05、0x06。

      // 开启显示
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24),
      static_cast<uint8_t>(RegisterOpcode::kWrite),
      0x002900,

      static_cast<uint8_t>(InitSequenceFormat::kDelayMs),
      10,

      // 全亮度可将寄存器 0x005100 设置为 0xFF。

  };

  int32_t rst_;
  uint16_t width_, height_;
  int16_t x_offset_, y_offset_;
  ColorFormat color_format_;
};
};  // namespace cpp_bus_driver
