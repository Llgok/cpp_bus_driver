/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:43:27
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Hi8561Touch final : public ChipI2cGuide {
 public:
  struct TouchInfo {
    uint16_t x = -1;              // x 坐标
    uint16_t y = -1;              // y 坐标
    uint8_t pressure_value = -1;  // 触摸压力值
  };

  struct TouchPoint {
    uint8_t finger_count = -1;     // 触摸手指总数
    bool edge_touch_flag = false;  // 边缘触摸标志

    std::vector<struct TouchInfo> info;
  };

  explicit Hi8561Touch(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;

  /**
   * @brief 获取触摸总数
   * @return
   * @Date 2025-03-28 09:51:25
   */
  uint8_t GetFingerCount();

  /**
   * @brief 获取单指触控的触摸点信息
   * @param &tp 使用结构体Touch_Point::配置触摸点结构体
   * @param finger_num 要获取的触摸点
   * @return [true]：获取的触摸点和finger_num相同
   * [false]：获取错误或者获取的触摸点和finger_num不相同
   * @Date 2025-03-28 09:49:03
   */
  bool GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num = 1);

  /**
   * @brief 获取多个触控的触摸点信息
   * @param &tp 使用结构体Touch_Point::配置触摸点结构体
   * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
   * @Date 2025-03-28 09:52:56
   */
  bool GetMultipleTouchPoint(TouchPoint& tp);

  /**
   * @brief 获取边缘检测
   * @return  [true]：屏幕边缘检测触发 [false]：屏幕边缘检测未触发
   * @Date 2025-03-28 09:56:59
   */
  bool GetEdgeTouch();

 private:
  static constexpr uint8_t kDeviceI2cAddressDefault = 0x68;
  static constexpr uint32_t kMemoryAddressEram = 0x20011000;
  static constexpr uint8_t kMaxDsramNum = 25;
  static constexpr uint32_t kDsramSectionInfoStartAddress =
      kMemoryAddressEram + 4;
  // 乘8bytes 是因为一共有两个数据，uint32_t数据（uint32_t地址（4
  // bytes）和uint32_t长度（4 bytes））
  static constexpr uint32_t kEsramNumStartAddress =
      kDsramSectionInfoStartAddress + kMaxDsramNum * 8;
  static constexpr uint32_t kEsramSectionInfoStartAddress =
      kEsramNumStartAddress + 4;
  static constexpr uint16_t kMemoryEramSize = 4 * 1024;
  static constexpr uint8_t kMaxTouchFingerCount = 10;
  static constexpr uint8_t kTouchPointAddressOffset = 3;
  static constexpr uint8_t kSingleTouchPointDataSize = 5;

  bool InitAddressInfo();

  int32_t rst_;
  uint32_t touch_info_start_address_;
};
}  // namespace cpp_bus_driver
