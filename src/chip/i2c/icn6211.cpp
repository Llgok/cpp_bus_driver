/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-01-16 11:57:07
 * @LastEditTime: 2026-04-23 17:15:51
 * @License: GPL 3.0
 */
#include "icn6211.h"

#include <cmath>

namespace cpp_bus_driver {
bool Icn6211::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetPinMode(rst_, PinMode::kOutput, PinStatus::kPullup);
    PinWrite(rst_, 1);
    DelayMs(10);
    PinWrite(rst_, 0);
    DelayMs(10);
    PinWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get icn6211 id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get icn6211 id success (id: %#X)\n", buffer);
  }

  return true;
}

uint16_t Icn6211::GetDeviceId() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceIdStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer[0] << 8 | buffer[1];
}

bool Icn6211::CheckInterfaceParamsOutOfRange(InterfaceParams& params) {
  bool result = true;

  // 检查并限制rgb_width (H Active Pixel) - 最大值4095
  if (params.rgb_width > 4095) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_width = 4095;
    result = false;
  } else if (params.rgb_width == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_width = 1;
    result = false;
  }

  // 检查并限制rgb_height (V Active Line) - 最大值4095
  if (params.rgb_height > 4095) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_height = 4095;
    result = false;
  } else if (params.rgb_height == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_height = 1;
    result = false;
  }

  // 检查并限制rgb_hfp (H Front Porch) - 最大值1023
  if (params.rgb_hfp > 1023) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_hfp = 1023;
    result = false;
  }

  // 检查并限制rgb_hsync (H Sync Width) - 最大值1023
  if (params.rgb_hsync > 1023) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_hsync = 1023;
    result = false;
  }

  // 检查并限制rgb_hbp (H Back Porch) - 最大值1023
  if (params.rgb_hbp > 1023) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_hbp = 1023;
    result = false;
  }

  // 检查并限制rgb_vfp (V Front Porch) - 最大值255
  if (params.rgb_vfp > 255) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_vfp = 255;
    result = false;
  }

  // 检查并限制rgb_vsync (V Sync Width) - 最大值255
  if (params.rgb_vsync > 255) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_vsync = 255;
    result = false;
  }

  // 检查并限制rgb_vbp (V Back Porch) - 最大值255
  if (params.rgb_vbp > 255) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    params.rgb_vbp = 255;
    result = false;
  }

  return result;
}

bool Icn6211::ConfigInterfaceParams(InterfaceParams params) {
  if (!CheckInterfaceParamsOutOfRange(params)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "CheckInterfaceParamsOutOfRange failed\n");
  }

  // 设置 H/V Active 低位
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kHactiveL),
          static_cast<uint8_t>(params.rgb_width))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kVactiveL),
          static_cast<uint8_t>(params.rgb_height))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 设置 H/V Active 高位
  uint8_t hv_h =
      ((params.rgb_height & 0x0F00) >> 4) | ((params.rgb_width & 0x0F00) >> 8);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kHvActiveH), hv_h)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 设置 kHfp/kHsync/kHbp 低位
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kHfpL),
          static_cast<uint8_t>(params.rgb_hfp))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kHsyncL),
          static_cast<uint8_t>(params.rgb_hsync))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kHbpL),
          static_cast<uint8_t>(params.rgb_hbp))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 设置 Horizontal Porch 高位
  uint8_t h_porch_h = ((params.rgb_hfp & 0x0300) >> 4) |
                      ((params.rgb_hsync & 0x0300) >> 6) |
                      ((params.rgb_hbp & 0x0300) >> 8);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kHPorchH), h_porch_h)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 设置 Vertical Porches
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kVfp),
          static_cast<uint8_t>(params.rgb_vfp))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kVsync),
          static_cast<uint8_t>(params.rgb_vsync))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kVbp),
          static_cast<uint8_t>(params.rgb_vbp))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 设置时钟相位
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kSysCtrl1),
          static_cast<uint8_t>(params.rgb_clock_phase))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 根据参考时钟设置选择时钟源
  if (params.external_reference_clock_mhz > 0) {
    // 使用外部参考时钟
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kPllRefSel),
            static_cast<uint8_t>(0x90))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
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

    if (!bus_->Write(static_cast<uint8_t>(Cmd::kPllRefDiv), pll_ref_div)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }

    // 向上取整
    uint8_t pll_int_value = static_cast<uint8_t>(ratio);
    if (ratio > static_cast<double>(pll_int_value)) {
      pll_int_value++;
    }

    if (!bus_->Write(static_cast<uint8_t>(Cmd::kPllInt), pll_int_value)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }

    LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
        "using external reference clock: %f mhz, kPllInt: %d\n",
        params.external_reference_clock_mhz, pll_int_value);
  } else {
    // 使用MIPI时钟作为参考
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kPllRefSel),
            static_cast<uint8_t>(0x92))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
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

    if (!bus_->Write(static_cast<uint8_t>(Cmd::kPllRefDiv), pll_ref_div)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }

    // 向上取整
    uint8_t pll_int_value = static_cast<uint8_t>(ratio);
    if (ratio > static_cast<double>(pll_int_value)) {
      pll_int_value++;
    }

    if (!bus_->Write(static_cast<uint8_t>(Cmd::kPllInt), pll_int_value)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }

    LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
        "using mipi clock: %f mhz, kPllInt: %d\n", params.mipi_clock_mhz,
        pll_int_value);
  }

  // 设置PLL相关寄存器
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kPllWtLock), static_cast<uint8_t>(0xFF))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kPllCtrl1), static_cast<uint8_t>(0x20))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 计算帧率
  fps_ =
      params.rgb_clock_mhz * 1000000.0 /
      ((params.rgb_width + params.rgb_hfp + params.rgb_hsync + params.rgb_hbp) *
          (params.rgb_height + params.rgb_vfp + params.rgb_vsync +
              params.rgb_vbp));

  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "ConfigInterfaceParams fps: %.03f\n", fps_);

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

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kSyncPolarityTestMode), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  return true;
}

bool Icn6211::SetMipiLane(uint8_t lane) {
  uint8_t buffer = 0x28 | ((lane - 1) & 0x03);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kDsiCtrl), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
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

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kSysCtrl0), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  return true;
}

bool Icn6211::SetTestMode(TestMode mode) {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kSyncPolarityTestMode), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (mode == TestMode::kDisable) {
    // 关闭 kBist
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kBistModeEn),
            static_cast<uint8_t>(0x83))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }

    if (!bus_->Write(static_cast<uint8_t>(Cmd::kSyncPolarityTestMode),
            static_cast<uint8_t>(0x00))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else {
    // 开启 kBist
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kBistModeEn),
            static_cast<uint8_t>(0x43))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  buffer = (buffer & 0B00000111) | static_cast<uint8_t>(mode);

  // 写入 kBist 模式
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kSyncPolarityTestMode), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Icn6211::SetChipEnable(bool enable) {
  uint8_t buffer = enable << 4;

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kConfigFinishSoftReset), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}
}  // namespace cpp_bus_driver