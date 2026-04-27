/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2026-04-20 14:44:28
 * @License: GPL 3.0
 */
#include "ft3x68.h"

namespace cpp_bus_driver {
bool Ft3x68::Init(int32_t freq_hz) {
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
        "Get ft3x68 id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get ft3x68 id success (id: %#X)\n", buffer);
  }

  return true;
}

uint8_t Ft3x68::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

uint8_t Ft3x68::GetFingerCount() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoTdStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Ft3x68::GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num) {
  if ((finger_num == 0) || (finger_num > kMaxTouchFingerCount)) {
    return false;
  }

  uint8_t buffer[kSingleTouchPointDataSize] = {0};

  // 地址自动偏移
  if (!bus_->Read(
          static_cast<uint8_t>(static_cast<uint8_t>(Cmd::kRoP1Xh) +
                               ((finger_num - 1) * kSingleTouchPointDataSize)),
          buffer, kSingleTouchPointDataSize)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  uint16_t buffer_x =
      (static_cast<uint16_t>(buffer[0] & 0B00001111) << 8) | buffer[1];
  uint16_t buffer_y =
      (static_cast<uint16_t>(buffer[2] & 0B00001111) << 8) | buffer[3];

  if ((buffer_x == static_cast<uint16_t>(-1)) &&
      (buffer_y == static_cast<uint16_t>(-1))) {
    return false;
  }

  tp.finger_count = finger_num;

  TouchInfo buffer_ti;
  buffer_ti.x = buffer_x;
  buffer_ti.y = buffer_y;

  tp.info.push_back(buffer_ti);

  return true;
}

bool Ft3x68::GetMultipleTouchPoint(TouchPoint& tp) {
  // +1 把手指数的地址也读出来
  const uint8_t buffer_touch_point_size =
      kMaxTouchFingerCount * kSingleTouchPointDataSize + 1;
  uint8_t buffer[buffer_touch_point_size] = {0};

  // 地址自动偏移
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoTdStatus), buffer,
          buffer_touch_point_size)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  // 如果手指数为0或者大于最大触摸手指数
  if ((buffer[0] == 0) || (buffer[0] > kMaxTouchFingerCount)) {
    return false;
  }
  tp.finger_count = buffer[0];

  for (uint8_t i = 0; i < tp.finger_count; i++) {
    const uint8_t buffer_touch_point_offset = i * kSingleTouchPointDataSize + 1;

    TouchInfo buffer_ti;
    buffer_ti.x =
        (static_cast<uint16_t>(buffer[buffer_touch_point_offset] & 0B00001111)
            << 8) |
        buffer[buffer_touch_point_offset + 1];
    buffer_ti.y = (static_cast<uint16_t>(
                       buffer[buffer_touch_point_offset + 2] & 0B00001111)
                      << 8) |
                  buffer[buffer_touch_point_offset + 3];

    tp.info.push_back(buffer_ti);
  }

  return true;
}

}  // namespace cpp_bus_driver
