/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-04-22 17:36:19
 * @License: GPL 3.0
 */
#include "hi8561_touch.h"

namespace cpp_bus_driver {
bool Hi8561Touch::Init(int32_t freq_hz) {
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

  if (!InitAddressInfo()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitAddressInfo failed\n");
    return false;
  } else {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "InitAddressInfo success\n");
  }

  return true;
}

bool Hi8561Touch::InitAddressInfo() {
  uint8_t buffer[] = {
      0xF3,
      static_cast<uint8_t>(kEsramSectionInfoStartAddress >> 24),
      static_cast<uint8_t>(kEsramSectionInfoStartAddress >> 16),
      static_cast<uint8_t>(kEsramSectionInfoStartAddress >> 8),
      static_cast<uint8_t>(kEsramSectionInfoStartAddress),
      0x03,
  };

  uint8_t buffer_2[48] = {0};

  if (!bus_->WriteRead(buffer, 6, buffer_2, 48)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  touch_info_start_address_ = buffer_2[8] + (buffer_2[8 + 1] << 8) +
                              (buffer_2[8 + 2] << 16) + (buffer_2[8 + 3] << 24);

  if ((touch_info_start_address_ < kMemoryAddressEram) ||
      (touch_info_start_address_ >= (kMemoryAddressEram + kMemoryEramSize))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "touch_info_start_address_ get error\n");
    touch_info_start_address_ = 0;
    return false;
  }

  return true;
}

uint8_t Hi8561Touch::GetFingerCount() {
  uint8_t buffer[] = {
      0xF3,
      static_cast<uint8_t>(touch_info_start_address_ >> 24),
      static_cast<uint8_t>(touch_info_start_address_ >> 16),
      static_cast<uint8_t>(touch_info_start_address_ >> 8),
      static_cast<uint8_t>(touch_info_start_address_),
      0x03,
  };

  uint8_t buffer_2 = 0;

  if (!bus_->WriteRead(buffer, 6, &buffer_2, 1)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteRead failed\n");
    return -1;
  }

  return buffer_2;
}

bool Hi8561Touch::GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num) {
  if ((finger_num == 0) || (finger_num > kMaxTouchFingerCount)) {
    return false;
  }

  uint8_t buffer[] = {
      0xF3,
      static_cast<uint8_t>(
          (touch_info_start_address_ + kTouchPointAddressOffset +
              (finger_num - 1) * kSingleTouchPointDataSize) >>
          24),
      static_cast<uint8_t>(
          (touch_info_start_address_ + kTouchPointAddressOffset +
              (finger_num - 1) * kSingleTouchPointDataSize) >>
          16),
      static_cast<uint8_t>(
          (touch_info_start_address_ + kTouchPointAddressOffset +
              (finger_num - 1) * kSingleTouchPointDataSize) >>
          8),
      static_cast<uint8_t>(
          (touch_info_start_address_ + kTouchPointAddressOffset +
              (finger_num - 1) * kSingleTouchPointDataSize)),
      0x03,
  };

  uint8_t buffer_2[kSingleTouchPointDataSize] = {0};

  if (!bus_->WriteRead(buffer, 6, buffer_2, kSingleTouchPointDataSize)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  uint16_t buffer_x = (static_cast<uint16_t>(buffer_2[0]) << 8) | buffer_2[1];
  uint16_t buffer_y = (static_cast<uint16_t>(buffer_2[2]) << 8) | buffer_2[3];

  if ((buffer_x == static_cast<uint16_t>(-1)) &&
      (buffer_y == static_cast<uint16_t>(-1))) {
    return false;
  }

  tp.finger_count = finger_num;

  TouchInfo buffer_ti;
  buffer_ti.x = buffer_x;
  buffer_ti.y = buffer_y;
  buffer_ti.pressure_value = buffer_2[4];

  tp.info.push_back(buffer_ti);

  return true;
}

bool Hi8561Touch::GetMultipleTouchPoint(TouchPoint& tp) {
  uint8_t buffer[] = {
      0xF3,
      static_cast<uint8_t>(touch_info_start_address_ >> 24),
      static_cast<uint8_t>(touch_info_start_address_ >> 16),
      static_cast<uint8_t>(touch_info_start_address_ >> 8),
      static_cast<uint8_t>(touch_info_start_address_),
      0x03,
  };

  const uint8_t buffer_touch_point_size =
      kTouchPointAddressOffset +
      kMaxTouchFingerCount * kSingleTouchPointDataSize;
  uint8_t buffer_2[buffer_touch_point_size] = {0};

  if (!bus_->WriteRead(buffer, 6, buffer_2, buffer_touch_point_size)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  // 如果手指数为0或者大于最大触摸手指数
  if ((buffer_2[0] == 0) || (buffer_2[0] > kMaxTouchFingerCount)) {
    return false;
  }
  tp.finger_count = buffer_2[0];

  for (uint8_t i = 0; i < tp.finger_count; i++) {
    const uint8_t buffer_touch_point_offset =
        kTouchPointAddressOffset + i * kSingleTouchPointDataSize;

    TouchInfo buffer_ti;
    buffer_ti.x =
        (static_cast<uint16_t>(buffer_2[buffer_touch_point_offset]) << 8) |
        buffer_2[buffer_touch_point_offset + 1];
    buffer_ti.y =
        (static_cast<uint16_t>(buffer_2[buffer_touch_point_offset + 2]) << 8) |
        buffer_2[buffer_touch_point_offset + 3];
    buffer_ti.pressure_value = buffer_2[buffer_touch_point_offset + 4];

    tp.info.push_back(buffer_ti);
  }

  if ((tp.info[tp.finger_count - 1].x == static_cast<uint16_t>(-1)) &&
      (tp.info[tp.finger_count - 1].y == static_cast<uint16_t>(-1)) &&
      (tp.info[tp.finger_count - 1].pressure_value == 0)) {
    tp.edge_touch_flag = true;
  } else {
    tp.edge_touch_flag = false;
  }

  return true;
}

bool Hi8561Touch::GetEdgeTouch() {
  uint8_t buffer[] = {
      0xF3,
      static_cast<uint8_t>(touch_info_start_address_ >> 24),
      static_cast<uint8_t>(touch_info_start_address_ >> 16),
      static_cast<uint8_t>(touch_info_start_address_ >> 8),
      static_cast<uint8_t>(touch_info_start_address_),
      0x03,
  };

  const uint8_t buffer_touch_point_size =
      kTouchPointAddressOffset +
      kMaxTouchFingerCount * kSingleTouchPointDataSize;
  uint8_t buffer_2[buffer_touch_point_size] = {0};

  if (!bus_->WriteRead(buffer, 6, buffer_2, buffer_touch_point_size)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  // 如果手指数为0
  if (buffer_2[0] == 0) {
    return false;
  }

  const uint8_t buffer_touch_point_offset =
      kTouchPointAddressOffset + buffer_2[0] * kSingleTouchPointDataSize -
      kSingleTouchPointDataSize;

  if ((static_cast<uint16_t>(
           (static_cast<uint16_t>(buffer_2[buffer_touch_point_offset]) << 8) |
           buffer_2[buffer_touch_point_offset + 1]) !=
          static_cast<uint16_t>(-1)) ||
      (static_cast<uint16_t>(
           (static_cast<uint16_t>(buffer_2[buffer_touch_point_offset + 2])
               << 8) |
           buffer_2[buffer_touch_point_offset + 3]) !=
          static_cast<uint16_t>(-1)) ||
      (buffer_2[buffer_touch_point_offset + 4] != 0)) {
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
