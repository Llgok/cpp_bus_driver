/*
 * @Description: gt9895
 * @Author: LILYGO_L
 * @Date 2025-07-09 09:15:31
 * @LastEditTime: 2026-04-30 13:43:17
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Gt9895 final : public ChipI2cGuide {
 public:
  struct TouchInfo {
    uint8_t finger_id = -1;       // 触摸手指id
    uint16_t x = -1;              // x 坐标
    uint16_t y = -1;              // y 坐标
    uint8_t pressure_value = -1;  // 触摸压力值
  };

  struct TouchPoint {
    uint8_t finger_count = -1;     // 触摸手指总数
    bool edge_touch_flag = false;  // 边缘触摸标志

    std::vector<struct TouchInfo> info;
  };

  explicit Gt9895(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE, float x_scale_factor = 1.0,
      float y_scale_factor = 1.0)
      : ChipI2cGuide(bus, address),
        rst_(rst),
        x_scale_factor_(x_scale_factor),
        y_scale_factor_(y_scale_factor) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;

  uint8_t GetDeviceId();

  /**
   * @brief 获取触摸总数
   * @return
   * @Date 2025-07-09 09:15:31
   */
  uint8_t GetFingerCount();

  /**
   * @brief 获取单指触控的触摸点信息
   * @param &tp 使用结构体Touch_Point::配置触摸点结构体
   * @param finger_num 要获取的触摸点
   * @return [true]：获取的触摸点和finger_num相同
   * [false]：获取错误或者获取的触摸点和finger_num不相同
   * @Date 2025-07-09 09:15:31
   */
  bool GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num = 1);

  /**
   * @brief 获取多个触控的触摸点信息
   * @param &tp 使用结构体Touch_Point::配置触摸点结构体
   * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
   * @Date 2025-07-09 09:15:31
   */
  bool GetMultipleTouchPoint(TouchPoint& tp);

  /**
   * @brief 获取边缘检测
   * @return  [true]：屏幕边缘检测触发 [false]：屏幕边缘检测未触发
   * @Date 2025-07-09 09:15:31
   */
  bool GetEdgeTouch();

  /**
   * @brief 设置睡眠
   * @return
   * @Date 2025-07-11 10:07:49
   */
  bool SetSleep();

 private:
  enum class Cmd {
    kRoIcInfoStartAddress = 0x00010070,  // ic信息开始地址

    kRoTouchInfoStartAddress = 0x00010308,  // 触摸信息开始地址

    kWoRealTimeCommandStartAddress = 0x00010174,  // 实时命令开始地址
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x5D;
  static constexpr uint8_t kDeviceId = 0xAD;
  static constexpr uint8_t kMaxTouchFingerCount = 10;
  static constexpr uint8_t kTouchPointAddressOffset = 8;
  static constexpr uint8_t kSingleTouchPointDataSize = 8;

  int32_t rst_;
  // 触摸xy坐标缩放处理比例
  float x_scale_factor_, y_scale_factor_;
};
}  // namespace cpp_bus_driver
