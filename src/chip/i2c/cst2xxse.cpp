/*
 * @Description: CST2xxSE 电容触摸控制器驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:51
 * @LastEditTime: 2026-05-16 23:45:48
 * @License: GPL 3.0
 */
#include "cst2xxse.h"

namespace cpp_bus_driver {
bool Cst2xxse::Init(int32_t freq_hz) {
  if (rst_ != kDefaultValue) {
    bool result = true;
    result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);
    result &= GpioWrite(rst_, 1);
    DelayMs(10);
    result &= GpioWrite(rst_, 0);
    DelayMs(10);
    result &= GpioWrite(rst_, 1);
    DelayMs(30);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get cst2xxse id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get cst2xxse id success (id: %#X)\n", buffer);
  }

  return true;
}

bool Cst2xxse::Deinit(bool delete_bus) {
  bool result = true;

  if (!ChipI2cGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
    result = false;
  }

  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }

  return result;
}

uint8_t Cst2xxse::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

uint8_t Cst2xxse::GetFingerCount() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetFingerCount), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer & 0B00001111;
}

bool Cst2xxse::GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num) {
  if ((finger_num == 0) || (finger_num > kMaxTouchFingerCount)) {
    return false;
  }

  std::array<uint8_t, kSingleTouchPointDataSize> buffer = {};

  if (finger_num == 1) {
    if (!bus_->Read(static_cast<uint8_t>(
                        static_cast<uint8_t>(Cmd::kRoTouchPointInfoStart) +
                        ((finger_num - 1) * kSingleTouchPointDataSize)),
            buffer.data(), buffer.size())) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
  } else {
    if (!bus_->Read(static_cast<uint8_t>(
                        static_cast<uint8_t>(Cmd::kRoTouchPointInfoStart) +
                        ((finger_num - 1) * kSingleTouchPointDataSize) + 2),
            buffer.data(), buffer.size())) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
  }

  uint16_t buffer_x =
      (static_cast<uint16_t>(buffer[1]) << 4) | ((buffer[3] & 0B11110000) >> 4);
  uint16_t buffer_y =
      (static_cast<uint16_t>(buffer[2]) << 4) | (buffer[3] & 0B00001111);

  if ((buffer_x == static_cast<uint16_t>(-1)) &&
      (buffer_y == static_cast<uint16_t>(-1))) {
    return false;
  }

  tp.finger_count = finger_num;

  TouchInfo buffer_ti;
  buffer_ti.x = buffer_x;
  buffer_ti.y = buffer_y;
  buffer_ti.pressure_value = buffer[4];

  tp.info.push_back(buffer_ti);

  return true;
}

bool Cst2xxse::GetMultipleTouchPoint(TouchPoint& tp) {
  const uint8_t buffer_touch_point_size =
      kMaxTouchFingerCount * kSingleTouchPointDataSize + 2;
  std::vector<uint8_t> buffer(buffer_touch_point_size, 0);

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoTouchPointInfoStart),
          buffer.data(), buffer_touch_point_size)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  // 如果手指数为0
  const uint8_t finger_count = buffer[5] & 0B00001111;
  if ((finger_count == 0) || (finger_count > kMaxTouchFingerCount)) {
    return false;
  }
  tp.finger_count = finger_count;

  for (uint8_t i = 0; i < tp.finger_count; i++) {
    uint8_t buffer_touch_point_offset;
    if (i == 1) {
      buffer_touch_point_offset = i * kSingleTouchPointDataSize;
    } else {
      buffer_touch_point_offset = i * kSingleTouchPointDataSize + 2;
    }

    TouchInfo buffer_ti;
    buffer_ti.x =
        (static_cast<uint16_t>(buffer[buffer_touch_point_offset + 1]) << 4) |
        ((buffer[buffer_touch_point_offset + 3] & 0B11110000) >> 4);
    buffer_ti.y =
        (static_cast<uint16_t>(buffer[buffer_touch_point_offset + 2]) << 4) |
        (buffer[buffer_touch_point_offset + 3] & 0B00001111);
    buffer_ti.pressure_value = buffer[buffer_touch_point_offset + 4];

    tp.info.push_back(buffer_ti);
  }

  if ((buffer[5] & 0B10000000) > 0) {
    tp.home_touch_flag = true;
  } else {
    tp.home_touch_flag = false;
  }

  return true;
}

bool Cst2xxse::GetHomeTouch() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetFingerCount), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if ((buffer & 0B10000000) == 0) {
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
