/*
 * @Description: 电容触摸控制器公共类型
 * @Author: LILYGO_L
 * @Date: 2026-08-11 00:00:00
 * @LastEditTime: 2026-08-11 00:00:00
 * @License: GPL 3.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace cpp_bus_driver {

constexpr size_t kMaxTouchContactCount = 10;
// 单帧触摸状态数据的字节数。
constexpr size_t kTouchStateSize = 5;

// 触摸帧读取结果。
enum class TouchReadStatus : uint8_t {
  kSuccess,
  kNoData,
  kBusError,
  kInvalidData,
};

// 触摸工具类型。
enum class TouchToolType : uint8_t {
  kFinger,
  kStylus,
  kStylusHover,
  kUnknown,
};

// 单个触摸接触点。
struct TouchContact {
  // 0 保留给上层表示无坐标的边缘事件，正常触点从 1 开始编号。
  uint8_t id = 0;
  uint16_t x = 0;
  uint16_t y = 0;
  uint16_t pressure = 0;
  TouchToolType tool = TouchToolType::kFinger;
};

// 一次硬件采样得到的完整触摸帧。
struct TouchFrame {
  std::array<TouchContact, kMaxTouchContactCount> contacts{};
  std::array<uint8_t, kTouchStateSize> state{};
  uint8_t contact_count = 0;
  uint8_t sequence = 0;
  uint8_t gesture = 0;
  uint8_t event_flags = 0;
  bool edge_touch = false;
};

// 原始触摸坐标到显示坐标的整数比例映射参数。
struct TouchCoordinateTransform {
  uint16_t source_width = 0;
  uint16_t source_height = 0;
  uint16_t target_width = 0;
  uint16_t target_height = 0;
};

}  // namespace cpp_bus_driver
