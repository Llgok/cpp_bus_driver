/*
 * @Description: ESP-IDF SDIO 主机通信驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-05-15 22:29:35
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace cpp_bus_driver {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
class HardwareSdio final : public BusSdioGuide {
 public:
  enum class SdioPort {
    kSlot0 = 0,  // 只能用作于固定GPIO口，专用于 Uhs-I 模式
    kSlot1 = 1,  // 可以通过GPIO交换矩阵路由，用于任意 GPIO口
  };

  explicit HardwareSdio(int32_t clk, int32_t cmd, int32_t d0,
      int32_t d1 = kDefaultValue,
      int32_t d2 = kDefaultValue,
      int32_t d3 = kDefaultValue,
      int32_t d4 = kDefaultValue,
      int32_t d5 = kDefaultValue,
      int32_t d6 = kDefaultValue,
      int32_t d7 = kDefaultValue,
      SdioPort port = SdioPort::kSlot1)
      : clk_(clk),
        cmd_(cmd),
        d0_(d0),
        d1_(d1),
        d2_(d2),
        d3_(d3),
        d4_(d4),
        d5_(d5),
        d6_(d6),
        d7_(d7),
        port_(port) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit() override;

  bool WaitInterrupt(uint32_t timeout_ms) override;

  /**
   * @brief 使用 IO_RW_EXTENDED (kCmd53) 的字节模式读多个字节
   * @param function 作用号
   * @param write_c32 读取的命令或地址
   * @param data 接收数据指针
   * @param byte 读取的数据长度，数据长度必须小于512个字节，且每4位内存要对齐
   * @return 读取成功返回 true，失败返回 false
   */
  bool Read(
      uint32_t function, uint32_t write_c32, void* data, size_t byte) override;

  /**
   * @brief 使用 IO_RW_DIRECT (kCmd52) 读单个字节
   * @param function 作用号
   * @param write_c32 读取的命令或地址
   * @param data 接收数据指针
   * @return 读取成功返回 true，失败返回 false
   */
  bool Read(uint32_t function, uint32_t write_c32, uint8_t* data) override;

  /**
   * @brief 块模式下，使用 IO_RW_EXTENDED (kCmd53) 读数据块
   * @param function 作用号
   * @param write_c32 读取的命令或地址
   * @param data 接收数据块指针
   * @param byte 读取的块数据长度，目前支持以最大512个字节每块数据传输
   * @return 读取成功返回 true，失败返回 false
   */
  bool ReadBlock(
      uint32_t function, uint32_t write_c32, void* data, size_t byte) override;

  /**
   * @brief 使用 IO_RW_EXTENDED (kCmd53) 的字节模式写多个字节
   * @param function 作用号
   * @param write_c32 写入的命令或地址
   * @param data 待写入数据指针
   * @param byte 写入的数据长度，数据长度必须小于512个字节，且每4位内存要对齐
   * @return 写入成功返回 true，失败返回 false
   */
  bool Write(uint32_t function, uint32_t write_c32, const void* data,
      size_t byte) override;

  /**
   * @brief 使用 IO_RW_DIRECT (kCmd52) 写单个字节
   * @param function 作用号
   * @param write_c32 写入的命令或地址
   * @param data 待写入字节
   * @param read_d8_verify 可选的回读校验结果指针，填 nullptr 表示禁用
   * @return 写入成功返回 true，失败返回 false
   */
  bool Write(uint32_t function, uint32_t write_c32, uint8_t data,
      uint8_t* read_d8_verify = nullptr) override;

  /**
   * @brief 块模式下，使用 IO_RW_EXTENDED (kCmd53) 写数据块
   * @param function 作用号
   * @param write_c32 写入的命令或地址
   * @param data 待写入数据块指针
   * @param byte 写入的块数据长度，目前支持以最大512个字节每块数据传输
   * @return 写入成功返回 true，失败返回 false
   */
  bool WriteBlock(uint32_t function, uint32_t write_c32, const void* data,
      size_t byte) override;

 private:
  static constexpr uint8_t kSdioBusInitTimeoutCount = 30;

  uint8_t width_ = 1;
  int32_t clk_, cmd_, d0_, d1_, d2_, d3_, d4_, d5_, d6_, d7_;
  SdioPort port_;
  int32_t freq_hz_ = kDefaultValue;
  std::unique_ptr<sdmmc_card_t> sdio_handle_;
  bool host_init_flag_ = false;
};
#endif
}  // namespace cpp_bus_driver
