/*
 * @Description: 软件模拟 I2C 总线驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-09-07 13:52:41
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>

#include "bus/bus_base.h"

namespace cpp_bus_driver {
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
class SoftwareI2c final : public I2cBusBase {
 public:
  explicit SoftwareI2c(int32_t sda, int32_t scl) : sda_(sda), scl_(scl) {}

  /**
   * @brief 初始化软件 I2C 总线
   * @param freq_hz 请求的时钟频率，范围 1-500000 Hz；实际频率受微秒级
   * 延时精度和 GPIO 操作开销影响，调用方应遵守从设备的时钟频率要求
   * @param address 7 位设备地址；kNoDeviceAddress 表示仅初始化总线
   * @return 初始化成功返回 true，参数无效或 GPIO 配置失败返回 false
   * @note 仅支持单主机，需要外部上拉电阻。所有操作限任务上下文使用。
   * 同一实例的操作通过互斥锁串行执行；共用引脚的不同实例需由调用方
   * 统一管理访问和初始化、释放时机。初始化不探测设备，也不自动恢复总线。
   */
  bool Init(uint32_t freq_hz = kDefaultFrequencyHz,
      uint16_t address = kNoDeviceAddress) override;

  /**
   * @brief 释放软件 I2C 总线
   * @param delete_bus true 表示复位 GPIO，false 表示保留 GPIO 配置并释放电平
   * @return 释放成功返回 true，GPIO 操作失败返回 false
   * @note 调用后需重新初始化才能通信；保留的 GPIO 可再次调用本函数复位。
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 从设备读取数据
   * @param data 接收缓冲区，长度非零时不可为空
   * @param length 读取字节数，零表示不发起通信
   * @return 读取成功返回 true，未就绪、参数无效或通信失败返回 false
   * @note 失败时缓冲区可能已写入部分数据。
   */
  bool Read(uint8_t* data, size_t length) override;

  /**
   * @brief 向设备写入数据
   * @param data 发送缓冲区，长度非零时不可为空
   * @param length 写入字节数，零表示不发起通信
   * @return 写入成功返回 true，未就绪、参数无效或通信失败返回 false
   * @note 失败时设备可能已接收部分数据，本函数不会自动重试。
   */
  bool Write(const uint8_t* data, size_t length) override;

  /**
   * @brief 在一次事务中写入并读取设备数据
   * @param write_data 发送缓冲区，写入长度非零时不可为空
   * @param write_length 写入字节数，零表示跳过写入阶段
   * @param read_data 接收缓冲区，读取长度非零时不可为空
   * @param read_length 读取字节数，零表示跳过读取阶段
   * @return 事务成功返回 true，未就绪、参数无效或通信失败返回 false
   * @note 两阶段之间使用重复起始信号；两长度均为零时不发起通信。
   * 失败时可能已经完成部分读写，本函数不会自动重试。
   */
  bool WriteRead(const uint8_t* write_data, size_t write_length,
      uint8_t* read_data, size_t read_length) override;

  /**
   * @brief 探测设备地址应答
   * @param address 待探测的 7 位设备地址，不改变初始化时绑定的地址
   * @return 设备应答且事务结束成功返回 true，否则返回 false
   */
  bool Probe(uint16_t address) override;

  /**
   * @brief 尝试恢复被从设备占用的总线
   * @return 总线恢复空闲返回 true，未初始化或无法恢复返回 false
   * @note 最多发送九个恢复时钟，再产生停止信号；不会重试设备读写。
   */
  bool RecoverBus();

 private:
  /**
   * @brief 执行完整的数据事务
   * @return 事务成功返回 true，参数无效或通信失败返回 false
   * @note 调用方必须持有实例互斥锁。
   */
  bool Transfer(const uint8_t* write_data, size_t write_length,
      uint8_t* read_data, size_t read_length);

  /**
   * @brief 产生起始或重复起始信号
   * @return 信号发送成功返回 true，总线被占用或 GPIO 操作失败返回 false
   */
  bool StartCondition();

  /**
   * @brief 产生停止信号并释放总线
   * @return 总线恢复空闲返回 true，否则返回 false
   */
  bool StopCondition();

  /**
   * @brief 清理事务并合并操作结果
   * @param success 清理前的操作是否成功
   * @return 操作与清理均成功返回 true，否则返回 false
   */
  bool FinishTransaction(bool success);

  /**
   * @brief 释放时钟线并等待实际高电平
   * @return 时钟拉高成功返回 true，超时或 GPIO 操作失败返回 false
   */
  bool RaiseClock();

  /**
   * @brief 发送一个数据位
   * @param high true 表示发送高电平，false 表示发送低电平
   * @return 发送成功返回 true，通信失败返回 false
   */
  bool WriteBit(bool high);

  /**
   * @brief 接收一个数据位
   * @param high 接收的电平，失败时保持原值
   * @return 接收成功返回 true，通信失败返回 false
   */
  bool ReadBit(bool& high);

  /**
   * @brief 发送一个字节并读取应答
   * @param data 发送的数据
   * @return 设备应答返回 true，未应答或通信失败返回 false
   */
  bool WriteByte(uint8_t data);

  /**
   * @brief 接收一个字节
   * @param data 接收的数据，失败时保持原值
   * @return 接收成功返回 true，通信失败返回 false
   * @note 应答位由事务流程另行发送。
   */
  bool ReadByte(uint8_t& data);

  /**
   * @brief 释放 SDA 和 SCL 的开漏输出
   * @return GPIO 操作均成功返回 true，否则返回 false
   */
  bool ReleaseLines();

  /**
   * @brief 复位已使用的总线引脚
   * @return GPIO 复位均成功返回 true，否则返回 false
   */
  bool ResetPins();

  // 默认总线时钟，单位 Hz。
  static constexpr uint32_t kDefaultFrequencyHz = 100000;
  // 最小半周期延时为 1 us，仅按延时计算的频率上限，单位 Hz。
  static constexpr uint32_t kMaximumFrequencyHz = 500000;
  // 单次等待 SCL 拉高的超时，单位 us。
  static constexpr int64_t kClockStretchTimeoutUs = 25000;

  const int32_t sda_;
  const int32_t scl_;
  uint16_t address_ = kNoDeviceAddress;
  uint32_t half_period_us_ = 5;
  bool initialized_ = false;
  // 包含初始化失败后或 Deinit(false) 保留的 GPIO。
  bool gpio_cleanup_required_ = false;
  bool transaction_active_ = false;
  std::mutex mutex_;
};
#endif
}  // namespace cpp_bus_driver
