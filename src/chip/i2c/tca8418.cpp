/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2026-04-21 11:47:45
 * @License: GPL 3.0
 */
#include "tca8418.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
constexpr const uint8_t Tca8418::kInitSequence[];
#endif

bool Tca8418::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    GpioWrite(rst_, 1);
    DelayMs(10);
    GpioWrite(rst_, 0);
    DelayMs(10);
    GpioWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  if (!InitSequence(kInitSequence, sizeof(kInitSequence))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  return true;
}

bool Tca8418::SetKeypadScanWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  // 有效性检查
  if (w == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (h == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (x >= kMaxWidthSize) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (y >= kMaxHeightSize) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (w > (kMaxWidthSize - x)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (h > (kMaxHeightSize - y)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  // 配置行选择寄存器
  uint8_t buffer_row_mask = 0;
  for (uint8_t i = y; i < (h + y); i++) {
    buffer_row_mask |= (1 << i);  // 设置对应的行位为1，键盘扫描
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoKpGpio1), buffer_row_mask)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 配置列选择寄存器
  uint8_t buffer_col_mask_low = 0;
  uint8_t buffer_col_mask_high = 0;
  for (uint8_t i = x; i < (w + x); i++) {
    if (i < 8) {
      buffer_col_mask_low |= (1 << i);  // 0~7列
    } else {
      buffer_col_mask_high |= (1 << (i - 8));  // 8，9列
    }
  }
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kWoKpGpio2), buffer_col_mask_low)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kWoKpGpio3), buffer_col_mask_high)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint8_t Tca8418::GetFingerCount() {
  uint8_t buffer = 0;

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRwKeyLockAndEventCounter), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer & 0x0F;
}

bool Tca8418::GetMultipleTouchPoint(TouchPoint& tp) {
  uint8_t buffer_finger_count = GetFingerCount();
  if ((buffer_finger_count == static_cast<uint8_t>(-1)) ||
      (buffer_finger_count == 0)) {
    return false;
  }

  uint8_t buffer[buffer_finger_count] = {0};

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  // 地址不能自动偏移
  for (size_t i = 0; i < buffer_finger_count; i++) {
    if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoKeyEvent), &buffer[i])) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
  }
#else
  // 地址自动偏移
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoKeyEvent), buffer,
          buffer_finger_count)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
#endif

  tp.finger_count = buffer_finger_count;

  for (uint8_t i = 0; i < tp.finger_count; i++) {
    TouchInfo buffer_ti;
    buffer_ti.press_flag = buffer[i] >> 7;
    buffer_ti.num = buffer[i] & 0B01111111;
    if (buffer_ti.num > 96) {
      buffer_ti.event_type = EventType::kGpio;
    }

    tp.info.push_back(buffer_ti);
  }

  return true;
}

uint8_t Tca8418::GetIrqFlag() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwInterruptStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer & 0B00011111;
}

bool Tca8418::ParseIrqStatus(uint8_t irq_flag, IrqStatus& status) {
  if (irq_flag == static_cast<uint8_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  status.ctrl_alt_del_key_sequence_flag = (irq_flag & 0B00010000) >> 4;
  status.fifo_overflow_flag = (irq_flag & 0B00001000) >> 3;
  status.keypad_lock_flag = (irq_flag & 0B00000100) >> 2;
  status.gpio_interrupt_flag = (irq_flag & 0B00000010) >> 1;
  status.key_events_flag = irq_flag & 0B00000001;

  return true;
}

bool Tca8418::ClearIrqFlag(IrqFlag flag) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwInterruptStatus),
          static_cast<uint8_t>(flag))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint32_t Tca8418::GetClearGpioIrqFlag() {
  uint8_t buffer[3] = {0};
  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoGpioInterruptStatusStart), buffer, 3)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return (static_cast<uint32_t>(buffer[0]) << 16) |
         (static_cast<uint32_t>(buffer[1]) << 8) |
         static_cast<uint32_t>(buffer[2]);
}

bool Tca8418::SetIrqGpioMode(IrqMask mode) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwConfiguration), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0B11110000) | static_cast<uint8_t>(mode);

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwConfiguration), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Tca8418::ParseTouchNum(uint8_t num, TouchPosition& position) {
  if (num == static_cast<uint8_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  if ((num == static_cast<uint8_t>(-1)) || (num == 0)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  position.x = (num - 1) % 10;
  position.y = (num - 1) / 10;

  return true;
}

}  // namespace cpp_bus_driver
