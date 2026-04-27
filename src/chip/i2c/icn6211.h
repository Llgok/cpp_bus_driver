
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-01-16 11:57:07
 * @LastEditTime: 2026-04-23 17:15:57
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Icn6211 final : public ChipI2cGuide {
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

  explicit Icn6211(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  uint16_t GetDeviceId();

  /**
   * @brief 检查接口参数是否正确
   * @param &params
   * 输入Interface_Params配置的接口参数，超出范围自动修改为极限值并返回false
   * @return
   * @Date 2026-01-16 17:34:46
   */
  bool CheckInterfaceParamsOutOfRange(InterfaceParams& params);

  /**
   * @brief 配置接口参数
   * @param params 使用Interface_Params::配置
   * @return
   * @Date 2026-01-16 17:34:29
   */
  bool ConfigInterfaceParams(InterfaceParams params);

  /**
   * @brief 配置信号极性
   * @param de de 信号极性
   * @param vsync vsync 信号极性
   * @param hsync hsync 信号极性
   * @return
   * @Date 2026-01-16 13:57:13
   */
  bool SetPolarityEnable(bool de, bool vsync, bool hsync);

  /**
   * @brief 设置mipi总线lane个数
   * @param lane 值范围：1~4
   * @return
   * @Date 2026-01-16 17:33:55
   */
  bool SetMipiLane(uint8_t lane);

  /**
   * @brief 设置rgb输出格式
   * @param format 使用Rgb_Format::配置
   * @param order 使用Rgb_Order::配置
   * @param rfc_enable [true]：开启，[false]：关闭
   * @return
   * @Date 2026-01-16 17:33:14
   */
  bool SetRgbOutputFormat(
      RgbFormat format, RgbOrder order, bool rfc_enable = false);

  /**
   * @brief 设置测试模式
   * @param mode 使用Test_Mode::配置
   * @return
   * @Date 2026-01-16 17:32:50
   */
  bool SetTestMode(TestMode mode);

  /**
   * @brief 设置芯片使能
   * @param enable [true]：开启，[false]：关闭
   * @return
   * @Date 2026-01-16 16:09:57
   */
  bool SetChipEnable(bool enable);

 private:
  enum class Cmd {
    kRoDeviceIdStart = 0x01,

    kConfigFinishSoftReset = 0x09,

    kSysCtrl0 = 0x10,
    kSysCtrl1,

    kBistModeEn = 0x14,

    kHactiveL = 0x20,
    kVactiveL,
    kHvActiveH,
    kHfpL,
    kHsyncL,
    kHbpL,
    kHPorchH,
    kVfp,
    kVsync,
    kVbp,
    kSyncPolarityTestMode,

    kPllCtrl1 = 0x51,

    kPllRefSel = 0x56,

    kPllWtLock = 0x5C,

    kPllInt = 0x69,

    kPllRefDiv = 0x6B,

    kMipiMode = 0x7A,

    kDsiCtrl = 0x86,
    kMipiPnSwap,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x2C;
  static constexpr uint16_t kDeviceId = 0x6211;

  int32_t rst_;
  double fps_;
};
}  // namespace cpp_bus_driver