/*
 * @Description: ICN6211 MIPI-DSI 转 RGB 桥接芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-01-16 11:57:07
 * @LastEditTime: 2026-08-03 16:11:17
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {
class Icn6211 final : public I2cChipBase {
 public:
  enum class RgbPhase {
    kPhase0 = 0x00,
    kPhase90 = 0x01,
    kPhase180 = 0x02,
    kPhase270 = 0x03
  };

  enum class RgbFormat {
    // kRgb666 格式
    kRgb666_50_50 = 0x00,  // GroupX[5:0] = Color[5:0]
    kRgb666_50_05 = 0x10,  // GroupX[5:0] = Color[0:5]
    kRgb666_72_50 = 0x20,  // GroupX[7:2] = Color[5:0]
    kRgb666_72_05 = 0x30,  // GroupX[7:2] = Color[0:5]

    // kRgb888 格式
    kRgb888_70_70 = 0x40,  // GroupX[7:0] = Color[7:0]
    kRgb888_70_07 = 0x50,  // GroupX[7:0] = Color[0:7]
  };

  enum class RgbOrder {
    kRgb = 0x00,  // Red(0) - Green(1) - Blue(2)
    kRbg = 0x01,  // Red(0) - Blue(1) - Green(2)
    kGrb = 0x02,  // Green(0) - Red(1) - Blue(2)
    kGbr = 0x03,  // Green(0) - Blue(1) - Red(2)
    kBrg = 0x04,  // Blue(0) - Red(1) - Green(2)
    kBgr = 0x05   // Blue(0) - Green(1) - Red(2)
  };

  enum class TestMode {
    kDisable = 0x00,
    kMonochrome = 0x18,
    kBorder = 0x28,
    kChessBoard = 0x38,
    kColorBar = 0x48,
    kColorSwitching = 0x58
  };

  struct InterfaceParams {
    uint16_t rgb_width;
    uint16_t rgb_height;
    uint16_t rgb_hfp;
    uint16_t rgb_hsync;
    uint16_t rgb_hbp;
    uint16_t rgb_vfp;
    uint16_t rgb_vsync;
    uint16_t rgb_vbp;
    double rgb_clock_mhz;  // kRgb 输出时钟
    RgbPhase rgb_clock_phase;

    double mipi_clock_mhz;  // kMipi 输入时钟

    // 外部参考时钟，设置为0则代表使用mipi时钟作为rgb信号时钟，设置非0则使用外部参考时钟作为rgb信号时钟
    double external_reference_clock_mhz = 0;
  };

  explicit Icn6211(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kPinNotConnected)
      : I2cChipBase(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取 ICN6211 芯片标识。
   * @return 芯片标识；读取失败返回 0xFFFF。
   */
  uint16_t GetChipId();

  /**
   * @brief 检查接口参数是否正确
   * @param params 接口参数；超出范围时会被限制到边界值并返回 false
   * @return 操作成功返回 true，失败返回 false
   */
  bool CheckInterfaceParamsOutOfRange(InterfaceParams& params);

  /**
   * @brief 配置接口参数
   * @param params 显示接口时序参数
   * @return 成功返回 true，失败返回 false
   */
  bool ConfigInterfaceParams(InterfaceParams params);

  /**
   * @brief 配置信号极性
   * @param de de 信号极性
   * @param vsync vsync 信号极性
   * @param hsync hsync 信号极性
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetPolarityEnable(bool de, bool vsync, bool hsync);

  /**
   * @brief 设置mipi总线lane个数
   * @param lane 值范围：1~4
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetMipiLane(uint8_t lane);

  /**
   * @brief 设置rgb输出格式
   * @param format RGB 数据格式
   * @param order RGB 通道顺序
   * @param rfc_enable [true]：开启，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRgbOutputFormat(
      RgbFormat format, RgbOrder order, bool rfc_enable = false);

  /**
   * @brief 设置测试模式
   * @param mode 内部测试图模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetTestMode(TestMode mode);

  /**
   * @brief 设置芯片使能
   * @param enable [true]：开启，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetChipEnable(bool enable);

 private:
  // 默认 I2C 总线时钟，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 100000;

  enum class Register {
    kRoChipIdStart = 0x01,
    kConfigFinishSoftReset = 0x09,
    kSysCtrl0 = 0x10,
    kSysCtrl1 = 0x11,
    kBistModeEn = 0x14,
    kHactiveL = 0x20,
    kVactiveL = 0x21,
    kHvActiveH = 0x22,
    kHfpL = 0x23,
    kHsyncL = 0x24,
    kHbpL = 0x25,
    kHPorchH = 0x26,
    kVfp = 0x27,
    kVsync = 0x28,
    kVbp = 0x29,
    kSyncPolarityTestMode = 0x2A,
    kPllCtrl1 = 0x51,
    kPllRefSel = 0x56,
    kPllWtLock = 0x5C,
    kPllInt = 0x69,
    kPllRefDiv = 0x6B,
    kMipiMode = 0x7A,
    kDsiCtrl = 0x86,
    kMipiPnSwap = 0x87,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x2C;
  static constexpr uint16_t kChipId = 0x6211;

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
