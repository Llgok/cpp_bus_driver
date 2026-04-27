
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-23 17:19:14
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

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  /**
   * @brief 设置睡眠
   * @param mode 使用Sleep_Mode::配置，睡眠模式
   * @param timeout_ms 超时时间，单位ms
   * @return
   * @Date 2025-05-29 18:36:08
   */
  bool SetSleep(SleepMode mode, int16_t timeout_ms = 100);

  /**
   * @brief 设置深度睡眠
   * @param sleep_time_ms 深度睡眠时间，单位ms
   * @param timeout_ms 超时时间，单位ms
   * @return
   * @Date 2026-04-15 16:52:23
   */
  bool SetDeepSleep(uint32_t sleep_time_ms, int16_t timeout_ms = 100);

  /**
   * @brief 初始化序列
   * @return
   * @Date 2025-05-09 18:03:26
   */
  bool InitSequence();

  /**
   * @brief 初次连接会返回准备完成信号
   * @return
   * @Date 2025-05-09 18:03:36
   */
  bool InitConnect();

  bool GetDeviceId();

  /**
   * @brief 重新连接
   * @return
   * @Date 2025-05-09 18:04:09
   */
  bool Reconnect();

  /**
   * @brief 获取连接状态
   * @return
   * @Date 2025-05-09 18:04:22
   */
  bool GetConnectStatus();

  /**
   * @brief 设置连接错误计数
   * @param count 错误计数的数字可以为正或者负
   * @return
   * @Date 2025-05-09 18:04:30
   */
  void SetConnectCount(int8_t count);

  /**
   * @brief 获取中断
   * @return
   * @Date 2025-03-21 17:11:27
   */
  uint32_t GetIrqFlag();

  /**
   * @brief 清除中断
   * @param irq_mask 要清除的中断请求位
   * @return
   * @Date 2025-03-21 17:11:35
   */
  bool ClearIrqFlag(uint32_t irq_mask);

  /**
   * @brief 解析接收到新包标志
   * @param flag 使用函数get_irq_flag()写入
   * @return [true]: 有接收到新的数据包，[false]: 没有接收到新的数据包
   * @Date 2025-03-21 17:21:59
   */
  bool ParseRxNewPacketFlag(uint32_t flag);

  /**
   * @brief 获取接收数据的长度
   * @return
   * @Date 2025-03-21 17:30:57
   */
  uint32_t GetRxDataLength();

  /**
   * @brief 接收包（只能进行小容量数据读取）
   * @param *data 包的数据容器
   * @return
   * @Date 2025-03-21 17:51:33
   */
  bool ReceivePacket(std::vector<uint8_t>& data);

  /**
   * @brief 接收包（配合heap_caps_malloc使用可以进行大容量数据读取）
   * @param *data 获取的数据指针
   * @param *byte 获取的数据长度
   * @return
   * @Date 2025-03-25 14:38:15
   */
  bool ReceivePacket(uint8_t* data, size_t* byte);

  /**
   * @brief 接收包（只能进行小容量数据读取）
   * @param &data 获取的数据指针
   * @param *byte 获取的数据长度
   * @return
   * @Date 2025-03-25 14:38:15
   */
  bool ReceivePacket(std::unique_ptr<uint8_t[]>& data, size_t* byte);

  /**
   * @brief 获取发送block数据缓冲区的长度
   * @return
   * @Date 2025-03-24 10:38:54
   */
  uint32_t GetTxBlockBufferLength();

  /**
   * @brief 发送包
   * @param *data 数据指针
   * @param byte 数据字节长度
   * @return
   * @Date 2025-03-24 10:47:22
   */
  bool SendPacket(const char* data, size_t byte);

  /**
   * @brief 发送包
   * @param data 需要发送的数据字符串
   * @return
   * @Date 2025-03-27 09:38:47
   */
  bool SendPacket(const std::string data);

  /**
   * @brief 设置wifi模式
   * @param mode 使用Wifi_Mode::配置
   * @param timeout_ms 超时时间，单位ms
   * @return
   * @Date 2025-03-26 14:03:40
   */
  bool SetWifiMode(WifiMode mode, int16_t timeout_ms = 100);

  /**
   * @brief
   * wifi扫描，使用之前需要调用函数set_wifi_mode()先将wifi模式设置为STATION模式
   * @param &data wifi_scan值的数据指针
   * @param timeout_ms 超时时间，单位ms
   * @return
   * @Date 2025-03-26 16:26:38
   */
  bool WifiScan(std::vector<uint8_t>& data, int16_t timeout_ms = 5000);

  /**
   * @brief 等待SDIO总线中断（使用前需要线开启SDIO总线中断）
   * @return
   * @Date 2025-03-27 09:00:05
   */
  bool WaitInterrupt(uint32_t timeout_ms);

  /**
   * @brief 设置保存到flash中
   * @param enable [true]：开启保存到falsh中 [false]：关闭保存到falsh中
   * @param timeout_ms 超时时间，单位ms
   * @return
   * @Date 2025-03-27 10:16:06
   */
  bool SetFlashSave(bool enable, int16_t timeout_ms = 100);

  /**
   * @brief 设置wifi连接
   * @param ssid wifi名字
   * @param password wifi密码
   * @param timeout_ms 超时时间，单位ms
   * @return
   * @Date 2025-03-27 10:17:09
   */
  bool SetWifiConnect(
      std::string ssid, std::string password = "", int16_t timeout_ms = 5000);

  /**
   * @brief 获取实时时间
   * @param &time 使用Real_Time结构体配置
   * @param timeout_ms 超时时间
   * @return
   * @Date 2025-05-06 16:07:50
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

    // 从从机那里接收到的总长度引索，初始化为0，如果该值和从机内部定义的不对应，将与从机失去连接，需要重新初始化
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
  int32_t rst_ = CPP_BUS_DRIVER_DEFAULT_VALUE;
  void (*rst_callback_)(bool value) = nullptr;
};
}  // namespace cpp_bus_driver