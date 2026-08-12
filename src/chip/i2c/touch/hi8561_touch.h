/*
 * @Description: HI8561 电容触摸控制器驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-08-11 00:00:00
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

#include "../../chip_guide.h"
#include "touch_types.h"

namespace cpp_bus_driver {

class Hi8561Touch final : public ChipI2cGuide {
 public:
  // HI8561 固件上报的手势编号。
  enum class Gesture : uint8_t {
    kNone = 0x00,
    kSwipeUp = 0x01,
    kSwipeDown = 0x02,
    kSwipeLeft = 0x03,
    kSwipeRight = 0x04,
    kLetterC = 0x20,
    kLetterZ = 0x21,
    kLetterM = 0x22,
    kLetterO = 0x23,
    kLetterS = 0x24,
    kLetterV = 0x25,
    kDoubleV = 0x26,
    kLeftV = 0x27,
    kRightV = 0x28,
    kLetterW = 0x29,
    kLetterE = 0x2A,
    kAt = 0x2B,
    kRightFold = 0x2C,
    kLeftFold = 0x2D,
    kTwoFingerSwipeUp = 0x51,
    kTwoFingerSwipeDown = 0x52,
    kTwoFingerSwipeLeft = 0x53,
    kTwoFingerSwipeRight = 0x54,
    kDoubleTap = 0x80,
  };

  // 屏幕方向对应的防误触边界模式。
  enum class RotationBorderMode : uint16_t {
    kPortrait = 0xA55A,
    kLandscapeNotchLeft = 0xA11A,
    kLandscapeNotchRight = 0xA33A,
  };

  // 固件和面板信息。
  struct FirmwareInfo {
    uint32_t customer_id_version = 0;
    uint32_t firmware_version = 0;
    uint16_t x_resolution = 0;
    uint16_t y_resolution = 0;
    uint8_t x_channel_count = 0;
    uint8_t y_channel_count = 0;
    uint8_t panel_maker = 0;
    uint8_t panel_version = 0;
    // 基础固件信息是否完整有效。
    bool valid = false;
  };

  explicit Hi8561Touch(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault, int32_t rst = kDefaultValue)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;

  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取供单点界面使用的首个触点
   * @param frame 接收首个触点、边缘事件、帧号和手势
   * @return 触摸数据读取状态
   * @note 该接口仅传输当前界面需要的最少字节，不执行完整报告校验和检查。
   */
  TouchReadStatus ReadPrimaryTouch(TouchFrame* frame);

  /**
   * @brief 一次读取完整触摸帧
   * @param frame 接收触点、边缘事件、帧号、手势和状态信息
   * @return 触摸帧读取状态
   */
  TouchReadStatus ReadTouchFrame(TouchFrame* frame);

  /**
   * @brief 按需读取固件、面板和坐标范围信息
   * @param firmware_info 接收读取到的固件和面板信息
   * @return 读取成功返回 true，参数无效、驱动未初始化或通信失败返回 false
   */
  bool ReadFirmwareInfo(FirmwareInfo* firmware_info);

  /**
   * @brief 启用或关闭高灵敏度模式
   * @param enabled 是否启用
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetHighSensitivityEnabled(bool enabled);

  /**
   * @brief 启用或关闭固件手势唤醒上报
   * @param enabled 是否启用
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGestureWakeEnabled(bool enabled);

  /**
   * @brief 通知触摸固件 USB 连接状态
   * @param connected USB 是否已连接
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetUsbConnected(bool connected);

  /**
   * @brief 设置屏幕方向对应的防误触边界模式
   * @param mode 防误触边界模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRotationBorderMode(RotationBorderMode mode);

  /**
   * @brief 通知触摸固件耳机连接状态
   * @param analog_connected 3.5 mm 耳机是否已连接
   * @param usb_connected Type-C 耳机是否已连接
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetEarphoneConnected(bool analog_connected, bool usb_connected);

  /**
   * @brief 启用或关闭虚拟接近检测
   * @param enabled 是否启用
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetVirtualProximityEnabled(bool enabled);

  /**
   * @brief 获取当前触摸扫描频带编号
   * @param frequency_band 接收频带编号
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetFrequencyBand(uint8_t* frequency_band);

 private:
  // 动态内存区的地址和长度。
  struct SectionInfo {
    uint32_t address = 0;
    uint32_t length = 0;
  };

  // 初始化阶段从固件描述表中发现的运行时地址。
  struct RuntimeAddresses {
    uint16_t dsram_section_count = 0;
    SectionInfo host;
    SectionInfo debug;
    SectionInfo coordinate_report;
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x68;
  static constexpr uint32_t kEramAddress = 0x20011000;
  static constexpr uint32_t kEramSize = 4 * 1024;
  static constexpr uint16_t kSectionReadyValue = 0xA55A;
  static constexpr size_t kMaxDsramSectionCount = 25;
  static constexpr size_t kMaxEsramSectionCount = 10;
  static constexpr uint32_t kDsramSectionTableAddress = kEramAddress + 4;
  static constexpr uint32_t kEsramCountAddress =
      kDsramSectionTableAddress + kMaxDsramSectionCount * 8;
  static constexpr uint32_t kEsramSectionTableAddress = kEsramCountAddress + 4;
  static constexpr size_t kDsramHostSectionIndex = 3;
  static constexpr size_t kDsramDebugSectionIndex = 4;
  static constexpr size_t kDsramFirmwareConfigSectionIndex = 1;
  static constexpr size_t kEsramCoordinateSectionIndex = 1;
  static constexpr size_t kTouchCoordinateOffset = 3;
  static constexpr size_t kTouchBytesPerContact = 5;
  static constexpr size_t kTouchStateOffset =
      kTouchCoordinateOffset +
      kMaxTouchContactCount * kTouchBytesPerContact;
  // 仅读取公开的触点和状态字段，不假设不同固件私有尾部的校验布局。
  static constexpr size_t kTouchReportReadSize =
      kTouchStateOffset + kTouchStateSize;
  // 触摸调试报告的最小输出间隔。
  static constexpr int64_t kDebugReportIntervalMs = 1000;

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
   * @brief 将原始触点记录解析为公共触点数据
   * @param data 指向一个 5 字节原始触点记录
   * @param contact_id 当前触点槽位对应的稳定编号
   * @param contact 接收解析后的公共触点数据
   * @return 触点有效且解析成功返回 true，无效触点或参数错误返回 false
   */
  static bool ParseContact(
      const uint8_t* data, uint8_t contact_id, TouchContact* contact);

  /**
   * @brief 读取并验证固件动态内存区描述表
   * @return 解析成功返回 true，失败返回 false
   */
  bool DiscoverRuntimeAddresses();

  /**
   * @brief 从指定固件配置区读取固件、面板和坐标范围信息
   * @param firmware_config_section 固件配置区描述
   * @param firmware_info 接收读取到的固件和面板信息
   * @return 读取成功返回 true，失败返回 false
   */
  bool ReadFirmwareInfoFromSection(
      const SectionInfo& firmware_config_section, FirmwareInfo* firmware_info);

  /**
   * @brief 进入或退出 HI8561 后门访问模式
   * @param enabled 是否进入后门访问模式
   * @return 模式切换成功返回 true，通信失败返回 false
   */
  bool SetBackdoorModeEnabled(bool enabled);

  /**
   * @brief 使用 HI8561 后门协议读取控制器内存
   * @param address 控制器内存起始地址
   * @param data 接收读取数据的缓冲区
   * @param length 需要读取的字节数
   * @return 读取成功返回 true，参数无效或通信失败返回 false
   */
  bool ReadBackdoorMemory(uint32_t address, uint8_t* data, size_t length);

  /**
   * @brief 使用 HI8561 后门读协议读取 ERAM
   * @param address ERAM 起始地址
   * @param data 接收读取数据的缓冲区
   * @param length 需要读取的字节数
   * @return 读取成功返回 true，参数越界或通信失败返回 false
   */
  bool ReadEram(uint32_t address, uint8_t* data, size_t length);

  /**
   * @brief 使用 HI8561 固件协议读取动态运行区
   * @param address 动态运行区起始地址
   * @param data 接收读取数据的缓冲区
   * @param length 需要读取的字节数
   * @return 读取成功返回 true，参数无效或通信失败返回 false
   */
  bool ReadRuntimeMemory(uint32_t address, uint8_t* data, size_t length);

  /**
   * @brief 使用 HI8561 固件协议写入动态运行区
   * @param address 动态运行区起始地址
   * @param data 待写入的数据
   * @param length 需要写入的字节数
   * @return 写入成功返回 true，参数无效或通信失败返回 false
   */
  bool WriteRuntimeMemory(uint32_t address, const uint8_t* data, size_t length);

  /**
   * @brief 读取动态内存区描述项
   * @param table_address 动态内存区描述表起始地址
   * @param section_count 描述表中有效项数量
   * @param section_index 需要读取的描述项索引
   * @param section 接收解析后的地址和长度
   * @return 描述项读取成功返回 true，参数越界或通信失败返回 false
   */
  bool ReadSectionInfo(uint32_t table_address, size_t section_count,
      size_t section_index, SectionInfo* section);

  /**
   * @brief 检查动态内存区能否容纳指定长度
   * @param section 待检查的动态内存区描述
   * @param required_length 调用方需要访问的最小长度
   * @return 地址和长度均有效返回 true，否则返回 false
   */
  static bool IsSectionValid(
      const SectionInfo& section, size_t required_length);

  /**
   * @brief 将 2 字节布尔开关写入固件 Host 区
   * @param offset 开关相对于 Host 区起始地址的偏移
   * @param enabled 是否启用对应功能
   * @param enabled_low_byte 启用状态写入的低字节
   * @param enabled_high_byte 启用状态写入的高字节
   * @return 写入成功返回 true，功能不受支持或通信失败返回 false
   */
  bool WriteHostBooleanSetting(size_t offset, bool enabled,
      uint8_t enabled_low_byte, uint8_t enabled_high_byte);

  int32_t rst_;
  RuntimeAddresses runtime_addresses_;
  // 串行化触摸报告、运行时设置和后门模式访问。
  std::mutex mutex_;
  // 最近一次输出触摸调试报告的时间。
  int64_t last_debug_report_ms_ = 0;
};

}  // namespace cpp_bus_driver
