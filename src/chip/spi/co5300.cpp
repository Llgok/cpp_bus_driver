/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-04-22 17:36:46
 * @License: GPL 3.0
 */
#include "co5300.h"

namespace cpp_bus_driver {
bool Co5300::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    GpioWrite(rst_, 1);
    DelayMs(10);
    GpioWrite(rst_, 0);
    DelayMs(10);
    GpioWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipQspiGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence) / sizeof(uint32_t))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  if (color_format_ != ColorFormat::kRgb565) {
    if (!SetColorFormat(color_format_)) {
      LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "SetColorFormat failed\n");
      return false;
    }
  }

  return true;
}

bool Co5300::Deinit() {
  if (!ChipQspiGuide::Deinit()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kDisable, GpioStatus::kDisable);
  }

  return true;
}

bool Co5300::SetRenderWindow(
    uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
  x_start += x_offset_;
  x_end += x_offset_;
  y_start += y_offset_;
  y_end += y_offset_;

  uint8_t buffer[] = {
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoColumnAddressSet) >> 16),
      static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoColumnAddressSet) >> 8),
      static_cast<uint8_t>(Reg::kWoColumnAddressSet),

      static_cast<uint8_t>(x_start >> 8),
      static_cast<uint8_t>(x_start),
      static_cast<uint8_t>(x_end >> 8),
      static_cast<uint8_t>(x_end),
  };
  uint8_t buffer_2[] = {
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoPageAddressSet) >> 16),
      static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoPageAddressSet) >> 8),
      static_cast<uint8_t>(Reg::kWoPageAddressSet),

      static_cast<uint8_t>(y_start >> 8),
      static_cast<uint8_t>(y_start),
      static_cast<uint8_t>(y_end >> 8),
      static_cast<uint8_t>(y_end),
  };
  uint8_t buffer_3[] = {
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryWriteStart) >> 16),
      static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryWriteStart) >> 8),
      static_cast<uint8_t>(Reg::kWoMemoryWriteStart),
  };

  if (!bus_->Write(buffer, 8)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(buffer_2, 8)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(buffer_3, 4)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Co5300::SendColorStream(
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data) {
  // 有效性检查
  if (data == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (w == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (h == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (x >= width_) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (y >= height_) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (w > (width_ - x)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (h > (height_ - y)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  // 硬件通常期望的是 [x_start, x_end] 和 [y_start, y_end] 的闭区间，即 x_end 和
  // y_end 是最后一个像素的坐标 例如： 如果 x=10, w=5，那么像素列是 10, 11, 12,
  // 13, 14，所以 x_end 应该是 14（即 x + w - 1） 如果不 -1，x_end 会是
  // 15，可能超出实际范围或导致多写一个像素
  if (!SetRenderWindow(x, x + w - 1, y, y + h - 1)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetRenderWindow failed\n");
    return false;
  }

  if (!SetWriteStreamMode(WriteStreamMode::kContinuousWrite4lanes)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetWriteStreamMode failed\n");
    return false;
  }

  if (color_format_ == ColorFormat::kRgb666) {
    if (!bus_->Write(
            data, w * h * 3, static_cast<uint32_t>(SpiTrans::kModeQio))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else {
    if (!bus_->Write(data, w * h * (static_cast<uint8_t>(color_format_) / 8),
            static_cast<uint32_t>(SpiTrans::kModeQio))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  return true;
}

bool Co5300::SetWriteStreamMode(WriteStreamMode mode) {
  uint8_t buffer[4] = {0};

  switch (mode) {
    case WriteStreamMode::kWrite1lanes:
      buffer[0] = static_cast<uint8_t>(Cmd::kWoWriteColorStream1lanesCmd);
      buffer[1] = static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryStartWrite) >> 16);
      buffer[2] = static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryStartWrite) >> 8);
      buffer[3] = static_cast<uint8_t>(Reg::kWoMemoryStartWrite);
      break;
    case WriteStreamMode::kWrite4lanes:
      buffer[0] = static_cast<uint8_t>(Cmd::kWoWriteColorStream4lanesCmd1);
      buffer[1] = static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryStartWrite) >> 16);
      buffer[2] = static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryStartWrite) >> 8);
      buffer[3] = static_cast<uint8_t>(Reg::kWoMemoryStartWrite);
      break;
    case WriteStreamMode::kContinuousWrite1lanes:
      buffer[0] = static_cast<uint8_t>(Cmd::kWoWriteColorStream1lanesCmd);
      buffer[1] = static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryContinuousWrite) >> 16);
      buffer[2] = static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryContinuousWrite) >> 8);
      buffer[3] = static_cast<uint8_t>(Reg::kWoMemoryContinuousWrite);
      break;
    case WriteStreamMode::kContinuousWrite4lanes:
      buffer[0] = static_cast<uint8_t>(Cmd::kWoWriteColorStream4lanesCmd1);
      buffer[1] = static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryContinuousWrite) >> 16);
      buffer[2] = static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoMemoryContinuousWrite) >> 8);
      buffer[3] = static_cast<uint8_t>(Reg::kWoMemoryContinuousWrite);
      break;

    default:
      break;
  }

  if (!bus_->Write(buffer, 4, static_cast<uint32_t>(NULL), true)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Co5300::SetBrightness(uint8_t value) {
  uint8_t buffer[] = {static_cast<uint8_t>(Cmd::kWoWriteRegister),
      static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoWriteDisplayBrightness) >> 16),
      static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoWriteDisplayBrightness) >> 8),
      static_cast<uint8_t>(Reg::kWoColumnAddressSet),

      static_cast<uint8_t>(value)};

  if (!bus_->Write(buffer, 5)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Co5300::SetSleep(bool enable) {
  uint8_t buffer[] = {
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoSleepIn) >> 16),
      static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoSleepIn) >> 8),
      static_cast<uint8_t>(Reg::kWoSleepIn),
  };

  if (enable) {
    buffer[1] =
        static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoSleepOut) >> 16);
    buffer[2] =
        static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoSleepOut) >> 8);
    buffer[3] = static_cast<uint8_t>(Reg::kWoSleepOut);
  }

  if (!bus_->Write(buffer, 4)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Co5300::SetScreenOff(bool enable) {
  uint8_t buffer[] = {
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoDisplayOff) >> 16),
      static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoDisplayOff) >> 8),
      static_cast<uint8_t>(Reg::kWoDisplayOff),
  };

  if (enable) {
    buffer[1] =
        static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoDisplayOn) >> 16);
    buffer[2] =
        static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoDisplayOn) >> 8);
    buffer[3] = static_cast<uint8_t>(Reg::kWoDisplayOn);
  }

  if (!bus_->Write(buffer, 4)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Co5300::SetColorEnhance(ColorEnhance mode) {
  uint8_t buffer[] = {
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoSetColorEnhance) >> 16),
      static_cast<uint8_t>(static_cast<uint32_t>(Reg::kWoSetColorEnhance) >> 8),
      static_cast<uint8_t>(Reg::kWoSetColorEnhance),

      static_cast<uint8_t>(mode),
  };

  if (!bus_->Write(buffer, 5)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Co5300::SetColorFormat(ColorFormat format) {
  uint8_t buffer[] = {
      static_cast<uint8_t>(Cmd::kWoWriteRegister),
      static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoInterfacePixelFormat) >> 16),
      static_cast<uint8_t>(
          static_cast<uint32_t>(Reg::kWoInterfacePixelFormat) >> 8),
      static_cast<uint8_t>(Reg::kWoInterfacePixelFormat),

      static_cast<uint8_t>(0x55),
  };

  switch (format) {
    case ColorFormat::kRgb565:
      break;
    case ColorFormat::kRgb666:
      buffer[4] = 0x66;
      break;
    case ColorFormat::kRgb888:
      buffer[4] = 0x77;
      break;

    default:
      return false;
  }

  if (!bus_->Write(buffer, 5)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
