/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:43:08
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Cst2xxse final : public ChipI2cGuide {
 public:
  struct TouchInfo {
    uint16_t x = -1;              // x 坐标
    uint16_t y = -1;              // y 坐标
    uint8_t pressure_value = -1;  // 触摸压力值
  };

  struct TouchPoint {
    uint8_t finger_count = -1;     // 触摸手指总数
    bool home_touch_flag = false;  // home按键触摸标志

    std::vector<struct TouchInfo> info;
  };

  explicit Cst2xxse(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;

  uint8_t GetDeviceId();

  /**
   * @brief 获取触摸总数
   * @return
   * @Date 2025-04-23 11:42:45
   */
  uint8_t GetFingerCount();

  /**
   * @brief 获取单指触控的触摸点信息
   * @param &tp 使用结构体Touch_Point::配置触摸点结构体
   * @param finger_num 要获取的触摸点
   * @return [true]：获取的触摸点和finger_num相同
   * [false]：获取错误或者获取的触摸点和finger_num不相同
   * @Date 2025-04-23 11:53:37
   */
  bool GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num = 1);

  /**
   * @brief 获取多个触控的触摸点信息
   * @param &tp 使用结构体Touch_Point::配置触摸点结构体
   * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
   * @Date 2025-04-23 15:05:49
   */
  bool GetMultipleTouchPoint(TouchPoint& tp);

  /**
   * @brief 获取home按键检测
   * @return  [true]：屏幕home按键检测触发 [false]：屏幕home按键检测未触发
   * @Date 2025-04-23 15:33:06
   */
  bool GetHomeTouch();

 private:
  enum class Cmd {
    kRoDeviceId = 0x06,  // 读取后返回0xAB
    kRoTouchPointInfoStart = 0x00,
    kRoGetFingerCount = 0x05,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x5A;
  static constexpr uint8_t kDeviceId = 0xAB;
  static constexpr uint8_t kMaxTouchFingerCount = 6;
  static constexpr uint8_t kSingleTouchPointDataSize = 5;

  int32_t rst_;
};
}  // namespace cpp_bus_driver
