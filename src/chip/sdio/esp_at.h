/*
 * @Description: 基于 SDIO 的 ESP-AT 通信驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-09-05 14:57:03
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "chip/chip_base.h"

namespace cpp_bus_driver {
class EspAt final : public SdioChipBase {
 public:
  enum class InterruptFlag {
    kRxNewPacket = 1 << 23,
  };

  explicit EspAt(std::shared_ptr<SdioBusBase> bus, int32_t rst)
      : SdioChipBase(bus), rst_(rst) {}

  explicit EspAt(
      std::shared_ptr<SdioBusBase> bus, void (*rst_callback)(bool value))
      : SdioChipBase(bus), rst_callback_(rst_callback) {}

  bool Init(int32_t freq_hz = kDefaultFrequencyKhz) override;
  bool Deinit() override;

  /**
   * @brief 发送 AT 探测命令并检查 ESP-AT 芯片响应。
   * @return 收到有效响应返回 true，否则返回 false。
   */
  bool GetChipId();

  /**
   * @brief 查询 ESP-AT SDIO 连接是否可用
   * @return 连接可用时返回 true，否则返回 false
   */
  bool IsConnected() const;

  /**
   * @brief 读取 ESP-AT 原始中断标志
   * @return 原始中断标志；读取失败时返回 UINT32_MAX
   */
  uint32_t GetInterruptFlags();

  /**
   * @brief 清除指定的 ESP-AT 中断标志
   * @param interrupt_flags 要清除的中断标志位
   * @return 操作成功返回 true，失败返回 false
   */
  bool ClearInterruptFlags(uint32_t interrupt_flags);

  /**
   * @brief 检查中断标志是否表示收到新数据包
   * @param interrupt_flags GetInterruptFlags() 返回的中断标志
   * @return 包含新数据包中断时返回 true，否则返回 false
   */
  bool HasReceivePacketInterrupt(uint32_t interrupt_flags) const;

  /**
   * @brief 获取当前可接收的数据长度
   * @return 可接收字节数；读取失败或无数据时返回 0
   */
  uint32_t GetReceiveDataLength();

  /**
   * @brief 使用调用方提供的缓冲区接收数据包
   * @param data 接收数据指针
   * @param byte 输入缓冲区容量，成功输出实际长度；容量不足输出所需长度且不读取
   * @return 操作成功返回 true，失败返回 false
   * @note
   * 其他失败输出长度为零；读总线失败后需重新初始化连接，缓冲区可能已部分写入
   */
  bool ReceivePacket(uint8_t* data, size_t* byte);

  /**
   * @brief 获取当前可用的发送缓冲区块数量
   * @return 可用发送缓冲区块数量；读取失败时返回 0
   */
  uint32_t GetTransmitBufferBlockCount();

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
   * @brief 等待 SDIO 总线中断
   * @param timeout_ms 等待超时时间，单位为毫秒
   * @return 等待成功返回 true，失败返回 false
   */
  bool WaitForInterrupt(uint32_t timeout_ms);

 private:
  // 默认 SDIO 总线时钟，单位 kHz。
  static constexpr int32_t kDefaultFrequencyKhz = 20000;

  enum class RegisterAddress {
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

  // 配置 ESP-AT 使用的 SDIO 功能和块大小。
  bool ConfigureSdioFunctions();

  // 等待 ESP-AT 启动完成通知。
  bool WaitForReady();

  /**
   * @brief 使用固定小块接收启动响应，并跨块查找目标文本
   * @param text 需要匹配的空字符结尾文本
   * @return 总超时内找到文本时返回 true
   */
  bool WaitForResponse(const char* text);

  /**
   * @brief 按已查询的长度读取数据，不申请堆内存
   * @param data 调用方缓冲区，至少可写 length 字节
   * @param length 已确认可读取的字节数，不得超过 SDIO 地址范围
   * @return 完整读取成功时返回 true
   * @note 查询和读取期间不得由其他任务并发接收同一连接
   */
  bool ReadPacketData(uint8_t* data, size_t length);

  // 更新底层传输错误计数和连接状态。
  void UpdateConnectionErrorCount(int8_t delta);

  static constexpr uint16_t kMaxTransmitBlockBufferSize = 512;
  // 地址由结束地址减去长度得到，单次操作禁止超出此范围。
  static constexpr size_t kMaxPacketSize =
      static_cast<size_t>(RegisterAddress::kSlaveCmd53EndAddr);
  static constexpr uint8_t kTxBufferOffset = 16;  // 发送缓冲区偏移量
  static constexpr uint16_t kTxBufferMask = 0xFFF;
  static constexpr uint32_t kRxBufferMask = 0xFFFFF;
  static constexpr uint32_t kRxBufferMax = 0x100000;
  static constexpr uint32_t kInvalidInterruptFlags = static_cast<uint32_t>(-1);
  static constexpr uint8_t kTransmitTimeoutCount = 100;
  static constexpr uint8_t kConnectErrorCount = 5;
  EspAtConnect connect_;
  int32_t rst_ = kPinNotConnected;
  void (*rst_callback_)(bool value) = nullptr;
};
}  // namespace cpp_bus_driver
