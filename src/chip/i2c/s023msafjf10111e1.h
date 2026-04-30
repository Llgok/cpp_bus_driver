/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-03-14 11:11:19
 * @LastEditTime: 2026-04-30 13:44:20
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class S023msafjf10111e1 final : public ChipI2cGuide {
 public:
  enum class DataFormat {
    kRgb888,
    kInternalTestMode,  // 内部测试图模式
  };

  enum class InternalTestMode {
    kColorBar = 0x80,
    kBlank = 0x82,
    kWhite = 0x92,
    kRed = 0xA2,
    kGreen = 0xB2,
    kBlue = 0xC2,
  };

  enum class ShowDirection {
    kNormal,
    kHorizontalMirror,          // 水平镜像
    kVerticalMirror,            // 垂直镜像
    kHorizontalVerticalMirror,  // 水平垂直镜像
  };

  explicit S023msafjf10111e1(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;

  /**
   * @brief 设置数据模式
   * @param format 使用Data_Format::配置
   * @return
   * @Date 2026-03-16 10:52:34
   */
  bool SetDataFormat(DataFormat format);

  /**
   * @brief 内部测试模式
   * @param mode 使用Internal_Test_Mode::配置
   * @return
   * @Date 2026-03-16 10:52:34
   */
  bool SetInternalTestMode(InternalTestMode mode);

  /**
   * @brief 设置显示方向
   * @param direction 使用Show_Direction::配置
   * @return
   * @Date 2026-03-16 10:52:34
   */
  bool SetShowDirection(ShowDirection direction);

  /**
   * @brief 设置亮度
   * @param value 值范围：0~511
   * @return
   * @Date 2026-03-16 10:52:34
   */
  bool SetBrightness(uint16_t value);

 private:
  enum class Cmd {
    kRwInternalTestModeRegisterControl1 = 0x0124,

    kRwInternalTestModeRegisterControl2 = 0x5128,

    kRwInternalTestMode = 0x5028,

    kRwDisplayBrightnessRegisterControl1 = 0x0124,

    kRwDisplayBrightnessRegisterControl2 = 0x0A28,

    kRwDisplayBrightness1 = 0x0328,

    kRwDisplayBrightness2 = 0x0428,

    kRwHorizontalVerticalMirror1 = 0x032C,

    kRwHorizontalVerticalMirror2 = 0x042C,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x54;

  int32_t rst_;
};
}  // namespace cpp_bus_driver
