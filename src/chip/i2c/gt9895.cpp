/*
 * @Description: gt9895
 * @Author: LILYGO_L
 * @Date 2025-07-09 09:15:31
 * @LastEditTime: 2026-04-20 14:44:50
 * @License: GPL 3.0
 */
#include "gt9895.h"

namespace cpp_bus_driver {
bool Gt9895::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    GpioWrite(rst_, 1);
    DelayMs(10);
    GpioWrite(rst_, 0);
    DelayMs(10);
    GpioWrite(rst_, 1);
    DelayMs(100);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get gt9895 id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get gt9895 id success (id: %#X)\n", buffer);
  }

  return true;
}

uint8_t Gt9895::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint32_t>(Cmd::kRoIcInfoStartAddress), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

uint8_t Gt9895::GetFingerCount() {
  uint8_t buffer[3] = {0};

  if (!bus_->Read(
          static_cast<uint32_t>(Cmd::kRoTouchInfoStartAddress), buffer, 3)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer[2];
}

bool Gt9895::GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num) {
  if ((finger_num == 0) || (finger_num > kMaxTouchFingerCount)) {
    return false;
  }

  const uint8_t buffer_touch_point_size =
      kTouchPointAddressOffset + finger_num * kSingleTouchPointDataSize;
  uint8_t buffer[buffer_touch_point_size] = {0};

  if (!bus_->Read(static_cast<uint32_t>(Cmd::kRoTouchInfoStartAddress), buffer,
          buffer_touch_point_size)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  // 如果手指数小于需要读取的手指数就读取失败
  if (buffer[2] < finger_num) {
    if (buffer[0] == 0x84) {
      tp.edge_touch_flag = true;

      tp.finger_count = 1;

      TouchInfo buffer_ti;
      buffer_ti.finger_id = 0;

      tp.info.push_back(buffer_ti);

      return true;
    } else {
      return false;
    }
  }
  tp.finger_count = buffer[2];

  const uint8_t buffer_touch_point_offset =
      kTouchPointAddressOffset + finger_num * kSingleTouchPointDataSize -
      kSingleTouchPointDataSize;

  TouchInfo buffer_ti;
  buffer_ti.finger_id = (buffer[buffer_touch_point_offset] >> 4) + 1;
  buffer_ti.x =
      (buffer[buffer_touch_point_offset + 2] |
          static_cast<uint16_t>(buffer[buffer_touch_point_offset + 3]) << 8) *
      x_scale_factor_;
  buffer_ti.y =
      (buffer[buffer_touch_point_offset + 4] |
          static_cast<uint16_t>(buffer[buffer_touch_point_offset + 5]) << 8) *
      y_scale_factor_;
  buffer_ti.pressure_value = buffer[buffer_touch_point_offset + 6];

  tp.info.push_back(buffer_ti);

  if (buffer[0] == 0x84) {
    tp.edge_touch_flag = true;
  } else {
    tp.edge_touch_flag = false;
  }

  return true;
}

bool Gt9895::GetMultipleTouchPoint(TouchPoint& tp) {
  const uint8_t buffer_touch_point_size =
      kTouchPointAddressOffset +
      kMaxTouchFingerCount * kSingleTouchPointDataSize;
  uint8_t buffer[buffer_touch_point_size] = {0};

  if (!bus_->Read(static_cast<uint32_t>(Cmd::kRoTouchInfoStartAddress), buffer,
          buffer_touch_point_size)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  // 如果手指数为0或者大于最大触摸手指数
  if (buffer[2] == 0) {
    if (buffer[0] == 0x84) {
      tp.edge_touch_flag = true;

      tp.finger_count = 1;

      TouchInfo buffer_ti;
      buffer_ti.finger_id = 0;

      tp.info.push_back(buffer_ti);

      return true;
    } else {
      return false;
    }
  } else if (buffer[2] > kMaxTouchFingerCount) {
    return false;
  }

  tp.finger_count = buffer[2];

  for (uint8_t i = 0; i < tp.finger_count; i++) {
    const uint8_t buffer_touch_point_offset =
        kTouchPointAddressOffset + i * kSingleTouchPointDataSize;

    TouchInfo buffer_ti;
    buffer_ti.finger_id = (buffer[buffer_touch_point_offset] >> 4) + 1;
    buffer_ti.x =
        (buffer[buffer_touch_point_offset + 2] |
            static_cast<uint16_t>(buffer[buffer_touch_point_offset + 3]) << 8) *
        x_scale_factor_;
    buffer_ti.y =
        (buffer[buffer_touch_point_offset + 4] |
            static_cast<uint16_t>(buffer[buffer_touch_point_offset + 5]) << 8) *
        y_scale_factor_;
    buffer_ti.pressure_value = buffer[buffer_touch_point_offset + 6];

    tp.info.push_back(buffer_ti);
  }

  if (buffer[0] == 0x84) {
    tp.edge_touch_flag = true;
  } else {
    tp.edge_touch_flag = false;
  }

  return true;
}

bool Gt9895::GetEdgeTouch() {
  uint8_t buffer = 0;

  if (!bus_->Read(
          static_cast<uint32_t>(Cmd::kRoTouchInfoStartAddress), &buffer, 1)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (buffer != 0x84) {
    return false;
  }

  return true;
}

bool Gt9895::SetSleep() {
  uint8_t buffer[] = {0x00, 0x00, 0x04, 0x84, 0x88, 0x00};

  if (!bus_->Write(static_cast<uint32_t>(Cmd::kWoRealTimeCommandStartAddress),
          buffer, 6)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
