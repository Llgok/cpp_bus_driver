/*
 * @Description: CST2xxSE 电容触摸控制器驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-08-03 16:11:07
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "chip/chip_base.h"

namespace cpp_bus_driver {
class Cst2xxse final : public I2cChipBase {
 public:
  struct TouchInfo {
    uint16_t x = -1;              // x 坐标
    uint16_t y = -1;              // y 坐标
    uint8_t pressure_value = -1;  // 触摸压力值
  };

  struct TouchPoint {
    uint8_t finger_count = -1;     // 触摸手指总数
    bool home_touch_flag = false;  // home按键触摸标志

    std::vector<TouchInfo> info;
  };

  explicit Cst2xxse(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kPinNotConnected)
      : I2cChipBase(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取 CST2xxSE 芯片标识。
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
   */
  bool GetSingleTouchPoint(TouchPoint& tp, uint8_t finger_num = 1);

  /**
   * @brief 获取多个触控的触摸点信息
   * @param tp 本次采样输出；调用时清除旧结果，保留容器容量
   * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
   */
  bool GetMultipleTouchPoint(TouchPoint& tp);

  /**
   * @brief 获取home按键检测
   * @return  [true]：屏幕home按键检测触发 [false]：屏幕home按键检测未触发
   */
  bool GetHomeTouch();

 private:
  // 默认 I2C 总线时钟，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 100000;

  enum class Register {
    kRoChipId = 0x06,  // 读取后返回0xAB
    kRoTouchPointInfoStart = 0x00,
    kRoGetFingerCount = 0x05,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x5A;
  static constexpr uint8_t kChipId = 0xAB;
  static constexpr uint8_t kMaxTouchFingerCount = 6;
  static constexpr uint8_t kSingleTouchPointDataSize = 5;

  /**
   * @brief 读取寄存器，并记录访问失败信息
   * @param reg 寄存器地址
   * @param data 接收缓冲区
   * @param length 读取字节数
   * @return 读取成功返回true，否则返回false
   */
  bool ReadRegister(uint8_t reg, uint8_t* data, size_t length = 1);

  int32_t rst_;
};
}  // namespace cpp_bus_driver
