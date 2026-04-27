
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-17 14:01:44
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Ft3x68 final : public ChipI2cGuide {
 public:
  struct TouchInfo {
    uint16_t x = -1;  // x 坐标
    uint16_t y = -1;  // y 坐标
  };

  struct TouchPoint {
    uint8_t finger_count = -1;  // 触摸手指总数

    std::vector<struct TouchInfo> info;
  };

  explicit Ft3x68(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  uint8_t GetDeviceId();

  /**
   * @brief 获取触摸总数
   * @return
   * @Date 2025-06-24 15:01:52
   */
  uint8_t GetFingerCount();

  /**
   * @brief 获取单指触控的触摸点信息
   * @param &tp 使用结构体TouchPoint::配置触摸点结构体
   * @param finger_num 要获取的触摸点
   * @return [true]：获取的触摸点和finger_num相同
   * [false]：获取错误或者获取的触摸点和finger_num不相同
   * @return
   * @Date 2025-06-24 15:47:40
   */
  bool GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num = 1);

  /**
   * @brief 获取多个触控的触摸点信息
   * @param &tp 使用结构体TouchPoint::配置触摸点结构体
   * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
   * @Date 2025-06-24 15:12:00
   */
  bool GetMultipleTouchPoint(TouchPoint& tp);

 private:
  enum class Cmd {
    // 0x00:kFt6456 0x04:kFt3268 0x01:kFt3067 0x05:kFt3368 0x02:kFt3068
    // 0x03:kFt3168
    kRoDeviceId = 0xA0,

    kRoTdStatus = 0x02,  // 触摸手指数
    kRoP1Xh,             // 第1点的X坐标高4位
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x38;
  static constexpr uint8_t kDeviceId = 0x03;
  static constexpr uint8_t kMaxTouchFingerCount = 2;
  static constexpr uint8_t kSingleTouchPointDataSize = 6;

  int32_t rst_;
};
}  // namespace cpp_bus_driver