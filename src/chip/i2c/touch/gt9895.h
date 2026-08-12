/*
 * @Description: GT9895 电容触摸控制器驱动接口
 * @Author: LILYGO_L
 * @Date: 2025-07-09 09:15:31
 * @LastEditTime: 2026-08-11 00:00:00
 * @License: GPL 3.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "../../chip_guide.h"
#include "touch_types.h"

namespace cpp_bus_driver {

class Gt9895 final : public ChipI2cGuide {
 public:
  static constexpr uint8_t kDefaultI2cAddress = 0x5D;
  static constexpr uint8_t kAlternateI2cAddress = 0x14;

  // 固件版本和芯片标识信息。
  struct ChipInfo {
    std::array<char, 7> rom_product_id{};
    std::array<char, 9> patch_product_id{};
    uint32_t rom_version = 0;
    uint32_t patch_version = 0;
    uint8_t sensor_id = 0;
  };

  explicit Gt9895(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDefaultI2cAddress, int32_t rst = kDefaultValue,
      int32_t irq = kDefaultValue,
      TouchCoordinateTransform coordinate_transform = {})
      : ChipI2cGuide(bus, address),
        i2c_address_(address),
        rst_(rst),
        irq_(irq),
        coordinate_transform_(coordinate_transform) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;

  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取供单点界面使用的首个触点
   * @param frame 接收首个触点和边缘事件
   * @return 触摸数据读取状态
   * @note 该接口校验事件头，但不会为校验完整触点区而读取其余无用触点。
   */
  TouchReadStatus ReadPrimaryTouch(TouchFrame* frame);

  /**
   * @brief 一次读取、校验并确认完整触摸帧
   * @param frame 接收触点、边缘事件和原始事件标志
   * @return 触摸帧读取状态
   */
  TouchReadStatus ReadTouchFrame(TouchFrame* frame);

  const ChipInfo& chip_info() const { return chip_info_; }

  /**
   * @brief 使触摸控制器进入睡眠模式
   * @return 命令发送成功返回 true，失败返回 false
   */
  bool EnterSleep();

  /**
   * @brief 使用中断唤醒时序和硬件复位唤醒触摸控制器
   * @return 唤醒成功返回 true，缺少复位引脚或操作失败返回 false
   */
  bool WakeUp();

 private:
  static constexpr uint32_t kFirmwareVersionAddress = 0x00010014;
  static constexpr uint32_t kCommandAddress = 0x00010174;
  static constexpr uint32_t kTouchEventAddress = 0x00010308;
  static constexpr uint16_t kExpectedProductId = 0x9895;
  static constexpr size_t kFirmwareInfoSize = 28;
  static constexpr size_t kEventHeaderSize = 8;
  static constexpr size_t kBytesPerContact = 8;
  static constexpr size_t kChecksumSize = 2;
  static constexpr size_t kPrefetchedContactCount = 2;
  static constexpr size_t kPrimaryReportSize =
      kEventHeaderSize + kBytesPerContact + kChecksumSize;
  static constexpr size_t kInitialReportSize =
      kEventHeaderSize + kPrefetchedContactCount * kBytesPerContact +
      kChecksumSize;
  static constexpr size_t kMaximumReportSize =
      kEventHeaderSize + kMaxTouchContactCount * kBytesPerContact +
      kChecksumSize;
  static constexpr size_t kReadAttemptCount = 2;
  static constexpr uint32_t kReadRetryDelayMs = 1;
  static constexpr size_t kChipInfoReadAttemptCount = 2;
  static constexpr uint32_t kChipInfoReadRetryDelayMs = 5;
  static constexpr int64_t kDebugReportIntervalMs = 1000;
  static constexpr int64_t kFailureReportIntervalMs = 1000;
  static constexpr uint8_t kTouchEventMask = 0x80;
  static constexpr uint8_t kEdgeTouchMask = 0x04;
  static constexpr uint8_t kStylusHoverType = 0x01;
  static constexpr uint8_t kStylusType = 0x03;

  /**
   * @brief 执行驱动可控的硬件复位时序
   * @return 无需控制复位引脚或复位成功返回 true，操作失败返回 false
   */
  bool ResetController();

  /**
   * @brief 读取并校验芯片固件版本和产品标识
   * @param chip_info 接收解析后的芯片固件版本和产品标识
   * @return 读取且校验成功返回 true，参数无效、通信失败或标识错误返回 false
   */
  bool ReadChipInfo(ChipInfo* chip_info);

  /**
   * @brief 从 32 位寄存器地址读取数据
   * @param address 32 位寄存器地址
   * @param data 接收读取数据的缓冲区
   * @param length 需要读取的字节数
   * @return 读取成功返回 true，参数无效或通信失败返回 false
   */
  bool ReadRegister(uint32_t address, uint8_t* data, size_t length);

  /**
   * @brief 清除当前已消费的触摸事件状态
   * @return 状态确认命令发送成功返回 true，失败返回 false
   */
  bool ClearTouchStatus();

  /**
   * @brief 读取并校验一帧稳定的 GT9895 触摸报告
   * @param max_contacts 本次最多解析的触点数量
   * @param frame 接收解析后的触摸帧
   * @return 触摸帧读取状态
   */
  TouchReadStatus ReadTouchReport(
      size_t max_contacts, TouchFrame* frame);

  /**
   * @brief 按限频策略输出完整触摸报告调试信息
   * @param mode 当前使用的触摸读取模式
   * @param report 原始触摸报告
   * @param report_size 原始触摸报告长度
   * @param reported_contact_count 固件上报的触点数量
   * @param frame 解析后的触摸帧
   */
  void LogTouchReport(const char* mode, const uint8_t* report,
      size_t report_size, uint8_t reported_contact_count,
      const TouchFrame& frame);

  /**
   * @brief 按限频策略输出校验失败的原始触摸报告
   * @param reason 校验失败原因
   * @param report 原始触摸报告
   * @param report_size 已读取的原始触摸报告长度
   * @param payload_size 触点数据及其校验值的预期长度
   */
  void LogInvalidTouchReport(const char* reason, const uint8_t* report,
      size_t report_size, size_t payload_size);

  /**
   * @brief 解析一个原始触点并执行可选的坐标变换
   * @param data 指向一个 8 字节原始触点记录
   * @param contact 接收解析后的公共触点数据
   */
  void ParseContact(const uint8_t* data, TouchContact* contact) const;

  /**
   * @brief 校验数据末尾保存的 16 位小端累加和
   * @param data 待校验的数据
   * @param length 包含末尾 2 字节校验值的数据长度
   * @return 校验通过返回 true，参数无效或校验失败返回 false
   */
  static bool HasValidChecksum(const uint8_t* data, size_t length);

  /**
   * @brief 检查坐标变换参数是否完整有效
   * @param transform 待检查的坐标变换参数
   * @return 源坐标范围和目标坐标范围均有效返回 true，否则返回 false
   */
  static bool IsCoordinateTransformValid(
      const TouchCoordinateTransform& transform);

  /**
   * @brief 将原始坐标按整数比例映射并钳位到目标范围
   * @param coordinate 原始坐标
   * @param source_extent 原始坐标轴长度
   * @param target_extent 目标坐标轴长度
   * @return 映射并钳位后的目标坐标
   */
  static uint16_t TransformCoordinate(
      uint16_t coordinate, uint16_t source_extent, uint16_t target_extent);

  int16_t i2c_address_;
  int32_t rst_;
  int32_t irq_;
  TouchCoordinateTransform coordinate_transform_;
  ChipInfo chip_info_;
  // 最近一次输出有效触摸调试报告的时间。
  int64_t last_debug_report_ms_ = 0;
  // 最近一次输出无效触摸报告的时间。
  int64_t last_failure_report_ms_ = 0;
};

}  // namespace cpp_bus_driver
