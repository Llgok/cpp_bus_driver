/*
 * @Description: SH8601 QSPI 显示控制器驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-08-03 16:11:56
 * @License: GPL 3.0
 */
#include "chip/spi/sh8601.h"

namespace cpp_bus_driver {
bool Sh8601::Init(int32_t freq_hz) {
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

  if (!QspiChipBase::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Init failed\n");
    return false;
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence) / sizeof(uint32_t))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "InitSequence failed\n");
    return false;
  }

  if (color_format_ != ColorFormat::kRgb565) {
    if (!SetColorFormat(color_format_)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "SetColorFormat failed\n");
      return false;
    }
  }

  return true;
}

bool Sh8601::Deinit() {
  bool result = true;

  if (!QspiChipBase::Deinit()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Deinit failed\n");
    result = false;
  }

  if (rst_ != kPinNotConnected) {
    result &= ResetGpio(rst_);
  }

  return result;
}

bool Sh8601::SetRenderWindow(int x_start, int y_start, int x_end, int y_end) {
  x_start += x_offset_;
  y_start += y_offset_;
  x_end += x_offset_;
  y_end += y_offset_;

  const uint8_t column[] = {static_cast<uint8_t>(x_start >> 8),
      static_cast<uint8_t>(x_start), static_cast<uint8_t>(x_end >> 8),
      static_cast<uint8_t>(x_end)};
  const uint8_t page[] = {static_cast<uint8_t>(y_start >> 8),
      static_cast<uint8_t>(y_start), static_cast<uint8_t>(y_end >> 8),
      static_cast<uint8_t>(y_end)};
  return WriteCommand(
             DcsCommand::kWoColumnAddressSet, column, sizeof(column)) &&
         WriteCommand(DcsCommand::kWoPageAddressSet, page, sizeof(page)) &&
         WriteCommand(DcsCommand::kWoMemoryWriteStart, nullptr, 0);
}

bool Sh8601::SendColorStream(
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data) {
  // 有效性检查
  if (data == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (w == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (h == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (x >= width_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (y >= height_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (w > (width_ - x)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (h > (height_ - y)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  // 硬件通常期望的是 [x_start, x_end] 和 [y_start, y_end] 的闭区间，即 x_end 和
  // y_end 是最后一个像素的坐标 例如： 如果 x=10, w=5，那么像素列是 10, 11, 12,
  // 13, 14，所以 x_end 应该是 14（即 x + w - 1） 如果不 -1，x_end 会是
  // 15，可能超出实际范围或导致多写一个像素
  if (!SetRenderWindow(x, y, x + w - 1, y + h - 1)) {
    return false;
  }

  if (!SetWriteStreamMode(WriteStreamMode::kContinuousWrite4Lanes)) {
    return false;
  }

  if (color_format_ == ColorFormat::kRgb666) {
    if (!bus_->Write(data, w * h * 3, static_cast<uint32_t>(SpiTrans::kModeQio),
            false)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "SH8601 pixel stream write failed (x: %u, y: %u, width: %u, height: "
          "%u)\n",
          static_cast<unsigned>(x), static_cast<unsigned>(y),
          static_cast<unsigned>(w), static_cast<unsigned>(h));
      return false;
    }
  } else {
    if (!bus_->Write(data, w * h * (static_cast<uint8_t>(color_format_) / 8),
            static_cast<uint32_t>(SpiTrans::kModeQio), false)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "SH8601 pixel stream write failed (x: %u, y: %u, width: %u, height: "
          "%u)\n",
          static_cast<unsigned>(x), static_cast<unsigned>(y),
          static_cast<unsigned>(w), static_cast<unsigned>(h));
      return false;
    }
  }

  return true;
}

bool Sh8601::SetWriteStreamMode(WriteStreamMode mode) {
  uint8_t buffer[4] = {0};

  switch (mode) {
    case WriteStreamMode::kWrite1Lane:
      buffer[0] = static_cast<uint8_t>(ColorStreamOpcode::kOneLane);
      buffer[1] = static_cast<uint8_t>(
          static_cast<uint32_t>(DcsCommand::kWoMemoryStartWrite) >> 16);
      buffer[2] = static_cast<uint8_t>(
          static_cast<uint32_t>(DcsCommand::kWoMemoryStartWrite) >> 8);
      buffer[3] = static_cast<uint8_t>(DcsCommand::kWoMemoryStartWrite);
      break;
    case WriteStreamMode::kWrite4Lanes:
      buffer[0] = static_cast<uint8_t>(ColorStreamOpcode::kFourLaneCommand1);
      buffer[1] = static_cast<uint8_t>(
          static_cast<uint32_t>(DcsCommand::kWoMemoryStartWrite) >> 16);
      buffer[2] = static_cast<uint8_t>(
          static_cast<uint32_t>(DcsCommand::kWoMemoryStartWrite) >> 8);
      buffer[3] = static_cast<uint8_t>(DcsCommand::kWoMemoryStartWrite);
      break;
    case WriteStreamMode::kContinuousWrite1Lane:
      buffer[0] = static_cast<uint8_t>(ColorStreamOpcode::kOneLane);
      buffer[1] = static_cast<uint8_t>(
          static_cast<uint32_t>(DcsCommand::kWoMemoryContinuousWrite) >> 16);
      buffer[2] = static_cast<uint8_t>(
          static_cast<uint32_t>(DcsCommand::kWoMemoryContinuousWrite) >> 8);
      buffer[3] = static_cast<uint8_t>(DcsCommand::kWoMemoryContinuousWrite);
      break;
    case WriteStreamMode::kContinuousWrite4Lanes:
      buffer[0] = static_cast<uint8_t>(ColorStreamOpcode::kFourLaneCommand1);
      buffer[1] = static_cast<uint8_t>(
          static_cast<uint32_t>(DcsCommand::kWoMemoryContinuousWrite) >> 16);
      buffer[2] = static_cast<uint8_t>(
          static_cast<uint32_t>(DcsCommand::kWoMemoryContinuousWrite) >> 8);
      buffer[3] = static_cast<uint8_t>(DcsCommand::kWoMemoryContinuousWrite);
      break;

    default:
      break;
  }

  if (!bus_->Write(buffer, 4, 0, true)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SH8601 stream command write failed (opcode: %#X, command: %#X)\n",
        static_cast<unsigned>(buffer[0]),
        (static_cast<unsigned>(buffer[1]) << 16) |
            (static_cast<unsigned>(buffer[2]) << 8) | buffer[3]);
    return false;
  }

  return true;
}

bool Sh8601::SetBrightness(uint8_t value) {
  return WriteCommand(
      DcsCommand::kWoWriteDisplayBrightness, &value, sizeof(value));
}

bool Sh8601::SetSleep(bool enable) {
  return WriteCommand(
      enable ? DcsCommand::kWoSleepOut : DcsCommand::kWoSleepIn, nullptr, 0);
}

bool Sh8601::SetScreenOff(bool enable) {
  return WriteCommand(
      enable ? DcsCommand::kWoDisplayOn : DcsCommand::kWoDisplayOff, nullptr,
      0);
}

bool Sh8601::SetColorEnhance(ColorEnhance mode) {
  const uint8_t value = static_cast<uint8_t>(mode);
  return WriteCommand(DcsCommand::kWoSetColorEnhance, &value, sizeof(value));
}

bool Sh8601::SetColorFormat(ColorFormat format) {
  uint8_t value = 0x55;
  switch (format) {
    case ColorFormat::kRgb565:
      break;
    case ColorFormat::kRgb666:
      value = 0x66;
      break;
    case ColorFormat::kRgb888:
      value = 0x77;
      break;
    default:
      return false;
  }
  return WriteCommand(
      DcsCommand::kWoInterfacePixelFormat, &value, sizeof(value));
}

bool Sh8601::WriteCommand(
    DcsCommand command, const uint8_t* data, size_t length) {
  uint8_t packet[8] = {static_cast<uint8_t>(CommandOpcode::kWrite),
      static_cast<uint8_t>(static_cast<uint32_t>(command) >> 16),
      static_cast<uint8_t>(static_cast<uint32_t>(command) >> 8),
      static_cast<uint8_t>(command)};
  if (length > sizeof(packet) - 4 || (length != 0 && data == nullptr)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid command write argument (command: %#X, length: %zu)\n",
        static_cast<unsigned>(command), length);
    return false;
  }
  for (size_t i = 0; i < length; ++i) {
    packet[4 + i] = data[i];
  }
  if (bus_ != nullptr && bus_->Write(packet, 4 + length, 0, false)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "SH8601 command write failed (command: %#X)\n",
      static_cast<unsigned>(command));
  return false;
}

}  // namespace cpp_bus_driver
