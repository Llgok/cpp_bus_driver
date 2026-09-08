/*
 * @Description: ICN6211 MIPI-DSI 转 RGB 桥接芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-01-16 11:57:07
 * @LastEditTime: 2026-09-02 16:15:33
 * @License: GPL 3.0
 */
#include "chip/i2c/icn6211.h"

#include <cmath>

namespace cpp_bus_driver {
bool Icn6211::Init(int32_t freq_hz) {
  if (rst_ != kPinNotConnected) {
    bool result = true;
    result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);
    result &= GpioWrite(rst_, 0);
    DelayMs(10);
    result &= GpioWrite(rst_, 1);
    DelayMs(10);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  }

  if (!I2cChipBase::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Init failed\n");
    return false;
  }

  auto buffer = GetChipId();
  if (buffer != kChipId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get icn6211 chip id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get icn6211 chip id success (id: %#X)\n", buffer);
  }

  return true;
}

bool Icn6211::Deinit(bool delete_bus) {
  bool result = true;

  if (!I2cChipBase::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Deinit failed\n");
    result = false;
  }

  if (rst_ != kPinNotConnected) {
    result &= ResetGpio(rst_);
  }

  return result;
}

uint16_t Icn6211::GetChipId() {
  uint8_t buffer[2] = {0};

  if (!ReadRegister(
          static_cast<uint8_t>(Register::kRoChipIdStart), buffer, 2)) {
    return -1;
  }

  return buffer[0] << 8 | buffer[1];
}

bool Icn6211::CheckInterfaceParamsOutOfRange(InterfaceParams& params) {
  bool result = true;

  // 检查并限制rgb_width (H Active Pixel) - 最大值4095
  if (params.rgb_width > 4095) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_width = 4095;
    result = false;
  } else if (params.rgb_width == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_width = 1;
    result = false;
  }

  // 检查并限制rgb_height (V Active Line) - 最大值4095
  if (params.rgb_height > 4095) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_height = 4095;
    result = false;
  } else if (params.rgb_height == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_height = 1;
    result = false;
  }

  // 检查并限制rgb_hfp (H Front Porch) - 最大值1023
  if (params.rgb_hfp > 1023) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_hfp = 1023;
    result = false;
  }

  // 检查并限制rgb_hsync (H Sync Width) - 最大值1023
  if (params.rgb_hsync > 1023) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_hsync = 1023;
    result = false;
  }

  // 检查并限制rgb_hbp (H Back Porch) - 最大值1023
  if (params.rgb_hbp > 1023) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_hbp = 1023;
    result = false;
  }

  // 检查并限制rgb_vfp (V Front Porch) - 最大值255
  if (params.rgb_vfp > 255) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_vfp = 255;
    result = false;
  }

  // 检查并限制rgb_vsync (V Sync Width) - 最大值255
  if (params.rgb_vsync > 255) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_vsync = 255;
    result = false;
  }

  // 检查并限制rgb_vbp (V Back Porch) - 最大值255
  if (params.rgb_vbp > 255) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_vbp = 255;
    result = false;
  }

  return result;
}

bool Icn6211::ConfigInterfaceParams(InterfaceParams params) {
  if (!CheckInterfaceParamsOutOfRange(params)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "CheckInterfaceParamsOutOfRange failed\n");
  }

  // 设置 H/V Active 低位
  if (!WriteRegister(static_cast<uint8_t>(Register::kHactiveL),
          static_cast<uint8_t>(params.rgb_width))) {
    return false;
  }
  if (!WriteRegister(static_cast<uint8_t>(Register::kVactiveL),
          static_cast<uint8_t>(params.rgb_height))) {
    return false;
  }

  // 设置 H/V Active 高位
  uint8_t hv_h =
      ((params.rgb_height & 0x0F00) >> 4) | ((params.rgb_width & 0x0F00) >> 8);
  if (!WriteRegister(static_cast<uint8_t>(Register::kHvActiveH), hv_h)) {
    return false;
  }

  // 设置 kHfp/kHsync/kHbp 低位
  if (!WriteRegister(static_cast<uint8_t>(Register::kHfpL),
          static_cast<uint8_t>(params.rgb_hfp))) {
    return false;
  }
  if (!WriteRegister(static_cast<uint8_t>(Register::kHsyncL),
          static_cast<uint8_t>(params.rgb_hsync))) {
    return false;
  }
  if (!WriteRegister(static_cast<uint8_t>(Register::kHbpL),
          static_cast<uint8_t>(params.rgb_hbp))) {
    return false;
  }

  // 设置 Horizontal Porch 高位
  uint8_t h_porch_h = ((params.rgb_hfp & 0x0300) >> 4) |
                      ((params.rgb_hsync & 0x0300) >> 6) |
                      ((params.rgb_hbp & 0x0300) >> 8);
  if (!WriteRegister(static_cast<uint8_t>(Register::kHPorchH), h_porch_h)) {
    return false;
  }

  // 设置 Vertical Porches
  if (!WriteRegister(static_cast<uint8_t>(Register::kVfp),
          static_cast<uint8_t>(params.rgb_vfp))) {
    return false;
  }
  if (!WriteRegister(static_cast<uint8_t>(Register::kVsync),
          static_cast<uint8_t>(params.rgb_vsync))) {
    return false;
  }
  if (!WriteRegister(static_cast<uint8_t>(Register::kVbp),
          static_cast<uint8_t>(params.rgb_vbp))) {
    return false;
  }

  // 设置时钟相位
  if (!WriteRegister(static_cast<uint8_t>(Register::kSysCtrl1),
          static_cast<uint8_t>(params.rgb_clock_phase))) {
    return false;
  }

  // 根据参考时钟设置选择时钟源
  if (params.external_reference_clock_mhz > 0) {
    // 使用外部参考时钟
    if (!WriteRegister(static_cast<uint8_t>(Register::kPllRefSel),
            static_cast<uint8_t>(0x90))) {
      return false;
    }

    // 计算外部参考时钟的PLL配置
    double ratio = params.rgb_clock_mhz / params.external_reference_clock_mhz;
    uint8_t pll_ref_div = 0;

    if (params.rgb_clock_mhz >= 87.5) {
      pll_ref_div = 0x31;  // 0b00110001
      ratio *= 8.0;
    } else if (params.rgb_clock_mhz >= 43.75) {
      pll_ref_div = 0x51;  // 0b01010001
      ratio *= 16.0;
    } else {
      pll_ref_div = 0x71;  // 0b01110001
      ratio *= 32.0;
    }

    if (!WriteRegister(
            static_cast<uint8_t>(Register::kPllRefDiv), pll_ref_div)) {
      return false;
    }

    // 向上取整
    uint8_t pll_int_value = static_cast<uint8_t>(ratio);
    if (ratio > static_cast<double>(pll_int_value)) {
      pll_int_value++;
    }

    if (!WriteRegister(
            static_cast<uint8_t>(Register::kPllInt), pll_int_value)) {
      return false;
    }

    LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
        "using external reference clock: %f mhz, kPllInt: %d\n",
        params.external_reference_clock_mhz, pll_int_value);
  } else {
    // 使用MIPI时钟作为参考
    if (!WriteRegister(static_cast<uint8_t>(Register::kPllRefSel),
            static_cast<uint8_t>(0x92))) {
      return false;
    }

    // 计算并设置 kPll 时钟（MIPI时钟）
    double ratio = params.rgb_clock_mhz / params.mipi_clock_mhz;
    uint8_t pll_ref_div = 0;

    if (params.rgb_clock_mhz >= 87.5) {
      pll_ref_div = 0x20;
      ratio *= 4;
    } else if (params.rgb_clock_mhz >= 43.75) {
      pll_ref_div = 0x40;
      ratio *= 8;
    } else {
      pll_ref_div = 0x60;
      ratio *= 16;
    }

    if (params.mipi_clock_mhz >= 320.0) {
      pll_ref_div |= 0x13;
      ratio *= 24.0;
    } else if (params.mipi_clock_mhz >= 160.0) {
      pll_ref_div |= 0x12;
      ratio *= 16.0;
    } else if (params.mipi_clock_mhz >= 80.0) {
      pll_ref_div |= 0x11;
      ratio *= 8.0;
    } else {
      pll_ref_div |= 0x01;
      ratio *= 4.0;
    }

    if (!WriteRegister(
            static_cast<uint8_t>(Register::kPllRefDiv), pll_ref_div)) {
      return false;
    }

    // 向上取整
    uint8_t pll_int_value = static_cast<uint8_t>(ratio);
    if (ratio > static_cast<double>(pll_int_value)) {
      pll_int_value++;
    }

    if (!WriteRegister(
            static_cast<uint8_t>(Register::kPllInt), pll_int_value)) {
      return false;
    }

    LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
        "using mipi clock: %f mhz, kPllInt: %d\n", params.mipi_clock_mhz,
        pll_int_value);
  }

  // 设置PLL相关寄存器
  if (!WriteRegister(static_cast<uint8_t>(Register::kPllWtLock),
          static_cast<uint8_t>(0xFF))) {
    return false;
  }

  if (!WriteRegister(static_cast<uint8_t>(Register::kPllCtrl1),
          static_cast<uint8_t>(0x20))) {
    return false;
  }

  // 计算帧率
  const double fps =
      params.rgb_clock_mhz * 1000000.0 /
      ((params.rgb_width + params.rgb_hfp + params.rgb_hsync + params.rgb_hbp) *
          (params.rgb_height + params.rgb_vfp + params.rgb_vsync +
              params.rgb_vbp));

  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "ConfigInterfaceParams fps: %.03f\n", fps);

  return true;
}

bool Icn6211::SetPolarityEnable(bool de, bool vsync, bool hsync) {
  uint8_t buffer = 0;
  if (de) {
    buffer |= 0x01;
  }
  if (vsync) {
    buffer |= 0x02;
  }
  if (hsync) {
    buffer |= 0x04;
  }

  if (!WriteRegister(
          static_cast<uint8_t>(Register::kSyncPolarityTestMode), buffer)) {
    return false;
  }
  return true;
}

bool Icn6211::SetMipiLane(uint8_t lane) {
  uint8_t buffer = 0x28 | ((lane - 1) & 0x03);

  if (!WriteRegister(static_cast<uint8_t>(Register::kDsiCtrl), buffer)) {
    return false;
  }

  return true;
}

bool Icn6211::SetRgbOutputFormat(
    RgbFormat format, RgbOrder order, bool rfc_enable) {
  uint8_t buffer = static_cast<uint8_t>(format) | static_cast<uint8_t>(order);

  if (rfc_enable) {
    buffer |= 0x80;
  }

  if (!WriteRegister(static_cast<uint8_t>(Register::kSysCtrl0), buffer)) {
    return false;
  }
  return true;
}

bool Icn6211::SetTestMode(TestMode mode) {
  uint8_t buffer = 0;

  if (!ReadRegister(
          static_cast<uint8_t>(Register::kSyncPolarityTestMode), &buffer)) {
    return false;
  }

  if (mode == TestMode::kDisable) {
    // 关闭 kBist
    if (!WriteRegister(static_cast<uint8_t>(Register::kBistModeEn),
            static_cast<uint8_t>(0x83))) {
      return false;
    }

    if (!WriteRegister(
            static_cast<uint8_t>(Register::kSyncPolarityTestMode),
            static_cast<uint8_t>(0x00))) {
      return false;
    }
  } else {
    // 开启 kBist
    if (!WriteRegister(static_cast<uint8_t>(Register::kBistModeEn),
            static_cast<uint8_t>(0x43))) {
      return false;
    }
  }

  buffer = (buffer & 0B00000111) | static_cast<uint8_t>(mode);

  // 写入 kBist 模式
  if (!WriteRegister(
          static_cast<uint8_t>(Register::kSyncPolarityTestMode), buffer)) {
    return false;
  }

  return true;
}

bool Icn6211::SetChipEnable(bool enable) {
  uint8_t buffer = enable << 4;

  if (!WriteRegister(
          static_cast<uint8_t>(Register::kConfigFinishSoftReset), buffer)) {
    return false;
  }

  return true;
}

bool Icn6211::ReadRegister(uint8_t reg, uint8_t* data, size_t length) {
  const uint8_t register_packet[] = {reg};

  if (bus_ != nullptr &&
      bus_->WriteRead(register_packet, sizeof(register_packet), data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "ICN6211 register read failed (register: %#X)\n",
      static_cast<unsigned>(reg));
  return false;
}

bool Icn6211::WriteRegister(uint8_t reg, uint8_t value) {
  const uint8_t register_packet[] = {reg, value};

  if (bus_ != nullptr &&
      bus_->Write(register_packet, sizeof(register_packet))) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "ICN6211 register write failed (register: %#X)\n",
      static_cast<unsigned>(reg));
  return false;
}
}  // namespace cpp_bus_driver
