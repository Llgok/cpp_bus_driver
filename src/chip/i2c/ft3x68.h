/*
 * @Description: FT3x68 电容触摸控制器驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-09-02 16:15:29
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "chip/chip_base.h"

namespace cpp_bus_driver {
class Ft3x68 final : public I2cChipBase {
 public:
  struct TouchInfo {
    uint16_t x = -1;  // x 坐标
    uint16_t y = -1;  // y 坐标
  };

  struct TouchPoint {
    uint8_t finger_count = -1;  // 触摸手指总数

    std::vector<TouchInfo> info;
  };

  explicit Ft3x68(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kPinNotConnected)
      : I2cChipBase(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取 FT3x68 芯片标识。
   * @return 芯片标识；读取失败返回 0xFF。
   */
  uint8_t GetChipId();

  /**
   * @brief 获取触摸总数
   * @return 返回读取到的数值
   */
  uint8_t GetFingerCount();

  /**
   * @brief 获取单指触控的触摸点信息
   * @param tp 本次采样输出；调用时清除旧结果，保留容器容量
   * @param finger_num 要获取的触摸点
   * @return [true]：获取的触摸点和finger_num相同
   * [false]：获取错误或者获取的触摸点和finger_num不相同
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num = 1);

  /**
   * @brief 获取多个触控的触摸点信息
   * @param tp 本次采样输出；调用时清除旧结果，保留容器容量
   * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
   */
  bool GetMultipleTouchPoint(TouchPoint& tp);

 private:
  // 默认 I2C 总线时钟，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 100000;

  enum class Register {
    // 芯片标识映射：0x00 为 kFt6456，0x04 为 kFt3268，
    // 0x01 为 kFt3067，0x05 为 kFt3368，0x02 为 kFt3068，0x03 为 kFt3168。
    kRoChipId = 0xA0,
    kRoTdStatus = 0x02,  // 触摸手指数
    kRoP1Xh = 0x03,             // 第1点的X坐标高4位
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x38;
  static constexpr uint8_t kChipId = 0x03;
  static constexpr uint8_t kMaxTouchFingerCount = 2;
  static constexpr uint8_t kSingleTouchPointDataSize = 6;

  int32_t rst_;
};
}  // namespace cpp_bus_driver
