/*
 * @Description: 基于 SDIO 的 ESP-AT 通信驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-05-15 00:03:48
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class EspAt final : public ChipSdioGuide {
 public:
  enum class IrqFlag {
    kRxNewPacket = 1 << 23,
  };

  enum class WifiMode {
    kOff,
    kStation,
    kSoftap,
    kStationSoftap,
  };

  enum class SleepMode {
    kDisableSleep,
    kModemSleep,
    kLightSleep,
    kModemSleepListenInterval,
    kPowerDown,  // 下电模式，将esp-at的使能引脚拉低
  };

  struct RealTime {
    std::string week = "";
    uint8_t day = -1;    // 日
    uint8_t month = -1;  // 月
    uint16_t year = -1;  // 年

    uint8_t hour = -1;    // 小时
    uint8_t minute = -1;  // 分钟
    uint8_t second = -1;  // 秒

    std::string time_zone = "";  // 时区
  };

  explicit EspAt(std::shared_ptr<BusSdioGuide> bus, int32_t rst)
      : ChipSdioGuide(bus), rst_(rst) {}

  explicit EspAt(
      std::shared_ptr<BusSdioGuide> bus, void (*rst_callback)(bool value))
      : ChipSdioGuide(bus), rst_callback_(rst_callback) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit() override;

  /**
   * @brief 设置睡眠
   * @param mode 睡眠模式
   * @param timeout_ms 超时时间，单位ms
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetSleep(SleepMode mode, int16_t timeout_ms = 100);

  /**
   * @brief 设置深度睡眠
   * @param sleep_time_ms 深度睡眠时间，单位ms
   * @param timeout_ms 超时时间，单位ms
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDeepSleep(uint32_t sleep_time_ms, int16_t timeout_ms = 100);

  /**
   * @brief 初始化序列
   * @return 初始化成功返回 true，失败返回 false
   */
  bool InitSequence();

  /**
   * @brief 初次连接会返回准备完成信号
   * @return 初始化成功返回 true，失败返回 false
   */
  bool InitConnect();
  bool GetDeviceId();

  /**
   * @brief 重新连接
   * @return 成功返回 true，失败返回 false
   */
  bool Reconnect();

  /**
   * @brief 获取连接状态
   * @return 条件满足返回 true，否则返回 false
   */
  bool GetConnectStatus();

  /**
   * @brief 设置连接错误计数
   * @param count 错误计数的数字可以为正或者负
   */
  void SetConnectCount(int8_t count);

  /**
   * @brief 获取中断
   * @return 返回读取到的数值
   */
  uint32_t GetIrqFlag();

  /**
   * @brief 清除中断
   * @param irq_mask 要清除的中断请求位
   * @return 操作成功返回 true，失败返回 false
   */
  bool ClearIrqFlag(uint32_t irq_mask);

  /**
   * @brief 解析接收到新包标志
   * @param flag GetIrqFlag() 返回的中断标志
   * @return true 表示收到新数据包，false 表示没有新数据包
   */
  bool ParseRxNewPacketFlag(uint32_t flag);

  /**
   * @brief 获取接收数据的长度
   * @return 返回读取到的数值
   */
  uint32_t GetRxDataLength();

  /**
   * @brief 使用字节容器接收小容量数据包
   * @param data 数据包容器
   * @return 操作成功返回 true，失败返回 false
   */
  bool ReceivePacket(std::vector<uint8_t>& data);

  /**
   * @brief 使用调用方提供的缓冲区接收数据包
   * @param data 接收数据指针
   * @param byte 缓冲区容量及实际数据长度指针
   * @return 操作成功返回 true，失败返回 false
   */
  bool ReceivePacket(uint8_t* data, size_t* byte);

  /**
   * @brief 分配智能指针缓冲区并接收数据包
   * @param data 接收数据的智能指针
   * @param byte 实际数据长度输出指针
   * @return 操作成功返回 true，失败返回 false
   */
  bool ReceivePacket(std::unique_ptr<uint8_t[]>& data, size_t* byte);

  /**
   * @brief 获取发送块缓冲区长度
   * @return 返回读取到的数值
   */
  uint32_t GetTxBlockBufferLength();

  /**
   * @brief 发送指定长度的字符数据包
   * @param data 待发送数据指针
   * @param byte 数据字节长度
   * @return 操作成功返回 true，失败返回 false
   */
  bool SendPacket(const char* data, size_t byte);

  /**
   * @brief 发送字符串数据包
   * @param data 需要发送的数据字符串
   * @return 操作成功返回 true，失败返回 false
   */
  bool SendPacket(const std::string& data);

  /**
   * @brief 设置wifi模式
   * @param mode Wi-Fi 工作模式
   * @param timeout_ms 超时时间，单位ms
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetWifiMode(WifiMode mode, int16_t timeout_ms = 100);

  /**
   * @brief 扫描 Wi-Fi；调用前需通过 SetWifiMode() 设置为站点模式
   * @param data 用于保存扫描响应数据的容器
   * @param timeout_ms 超时时间，单位为毫秒
   * @return 成功返回 true，失败返回 false
   */
  bool WifiScan(std::vector<uint8_t>& data, int16_t timeout_ms = 5000);

  /**
   * @brief 等待SDIO总线中断（使用前需要线开启SDIO总线中断）
   * @param timeout_ms 等待超时时间，单位为毫秒
   * @return 等待成功返回 true，失败返回 false
   */
  bool WaitInterrupt(uint32_t timeout_ms);

  /**
   * @brief 设置保存到flash中
   * @param enable true 表示保存到闪存，false 表示不保存到闪存
   * @param timeout_ms 超时时间，单位ms
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetFlashSave(bool enable, int16_t timeout_ms = 100);

  /**
   * @brief 设置wifi连接
   * @param ssid wifi名字
   * @param password wifi密码
   * @param timeout_ms 超时时间，单位ms
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetWifiConnect(
      std::string ssid, std::string password = "", int16_t timeout_ms = 5000);

  /**
   * @brief 获取实时时间
   * @param time 用于保存结果的 RealTime 结构体
   * @param timeout_ms 超时时间
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetRealTime(RealTime& time, int16_t timeout_ms = 3000);

 private:
  enum class Cmd {
    kSdIoCccrFnEnable = 0x00000002,
    kSdIoCccrFnReady,
    kSdIoCccrIntEnable,

    kSdIoCccrBusWidth = 0x00000007,
    kSdIoCccrBlksizel = 0x00000010,
    kSdIoCccrBlksizeh,

    kSlchostBase = 0x3FF55000,
    kSlaveCmd53EndAddr = 0x1F800,

    kPacketLength = (kSlchostBase + 0x60) & 0x3FF,
    kInterruptClear = (kSlchostBase + 0xD4) & 0x3FF,
    kInterruptRaw = (kSlchostBase + 0x50) & 0x3FF,  // 原始中断位
    kInterruptSt = (kSlchostBase + 0x58) & 0x3FF,   // 掩码中断位
    kTokenRdata = (kSlchostBase + 0x44) & 0x3FF,
    kConf = (kSlchostBase + 0x8C) & 0x3FF,
    kConfOffset = 0,
  };

  struct EspAtConnect {
    // 设备连接状态
    bool status = true;
    int8_t error_count = 0;

    // 从设备接收的总长度索引；与从设备内部值不一致时需重新初始化连接。
    uint32_t receive_total_length_index = 0;
  };

  static constexpr uint16_t kMaxTransmitBlockBufferSize = 512;
  static constexpr uint8_t kTxBufferOffset = 16;  // 发送缓冲区偏移量
  static constexpr uint16_t kTxBufferMask = 0xFFF;
  static constexpr uint32_t kRxBufferMask = 0xFFFFF;
  static constexpr uint32_t kRxBufferMax = 0x100000;
  static constexpr uint8_t kTransmitTimeoutCount = 100;
  static constexpr uint8_t kConnectErrorCount = 5;
  static constexpr const char* kTimeMonthTable_[] = {"Jan", "Feb", "Mar", "Apr",
      "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  EspAtConnect connect_;
  int32_t rst_ = kDefaultValue;
  void (*rst_callback_)(bool value) = nullptr;
};
}  // namespace cpp_bus_driver
