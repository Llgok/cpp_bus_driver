
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2026-04-17 09:15:10
 * @License: GPL 3.0
 */
#pragma once

#include "../config.h"

namespace cpp_bus_driver {
class BusI2cGuide : public Tool {
 public:
  BusI2cGuide() = default;

  virtual bool Init(uint32_t freq_hz, uint16_t address) = 0;
  virtual bool Read(uint8_t* data, size_t length) = 0;
  virtual bool Write(const uint8_t* data, size_t length) = 0;
  virtual bool WriteRead(const uint8_t* write_data, size_t write_length,
      uint8_t* read_data, size_t read_length) = 0;
  virtual bool Probe(const uint16_t address) = 0;

  virtual bool Deinit();

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  virtual i2c_cmd_handle_t CmdLinkCreate();
  virtual bool StartTransmit(
      i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en = true);
  virtual bool Write(
      i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en = true);
  virtual bool Write(i2c_cmd_handle_t cmd_handle, const uint8_t* data,
      size_t data_len, bool ack_en = true);
  virtual bool Read(i2c_cmd_handle_t cmd_handle, uint8_t* data, size_t data_len,
      i2c_ack_type_t ack = I2C_MASTER_LAST_NACK);
  virtual bool StopTransmit(i2c_cmd_handle_t cmd_handle);

  virtual bool StartTransmit();
  virtual bool StopTransmit();
#endif

  bool Read(
      const uint8_t write_c8, uint8_t* read_data, size_t read_data_length = 1);
  bool Read(const uint16_t write_c16, uint8_t* read_data,
      size_t read_data_length = 1);
  bool Read(const uint32_t write_c32, uint8_t* read_data,
      size_t read_data_length = 1);
  bool Write(const uint8_t write_c8, const uint8_t write_d8);
  bool Write(const uint8_t write_c8, const uint16_t write_d16,
      Endian endian = Endian::kBig);
  bool Write(const uint16_t write_c16, const uint8_t write_d8);
  bool Write(const uint8_t write_c8, const uint8_t* write_data,
      size_t write_data_length);
  bool Write(const uint32_t write_c32, const uint8_t* write_data,
      size_t write_data_length);

  bool Scan7bitAddress(std::vector<uint8_t>* address);
};

class BusI2sGuide : public Tool {
 public:
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  enum class DataMode {
    kInput,   // 输入模式
    kOutput,  // 输出模式

    kInputOutput,  // 输入输出共有
  };
#endif

  BusI2sGuide() = default;

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  virtual bool Init(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz,
      i2s_data_bit_width_t data_bit_width) = 0;

  virtual size_t Read(void* data, size_t byte) = 0;
  virtual size_t Write(const void* data, size_t byte) = 0;

  virtual bool SetClockReconfig(i2s_mclk_multiple_t mclk_multiple,
      uint32_t sample_rate_hz, DataMode data_mode = DataMode::kInputOutput) = 0;
  virtual bool SetChannelEnable(
      bool enable, DataMode data_mode = DataMode::kInputOutput) = 0;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  virtual bool Init(nrf_i2s_ratio_t mclk_multiple, uint32_t sample_rate_hz,
      nrf_i2s_swidth_t data_bit_width) = 0;

  /**
   * @brief 数据流传输开始
   * @param *write_data
   * 写数据流缓存指针，如果为nullptr表示不写入数据，*write_data需要使用ram分配的内存
   * @param *read_data
   * 读数据流缓存指针，如果为nullptr表示不读取数据，*read_data需要使用ram分配的内存
   * @param max_data_length 数据流缓存最大长度
   * @return
   * @Date 2025-08-29 17:49:07
   */
  virtual bool StartTransmit(
      uint32_t* write_data, uint32_t* read_data, size_t max_data_length) = 0;

  /**
   * @brief 停止数据流传输
   * @return
   * @Date 2025-08-29 17:51:03
   */
  virtual void StopTransmit() = 0;

  /**
   * @brief 设置下一个读取的指针
   * @param *data 数据指针
   * @return
   * @Date 2025-08-29 17:52:08
   */
  virtual bool SetNextRead(uint32_t* data) = 0;

  /**
   * @brief 设置下一个写入的指针
   * @param *data 数据指针
   * @return
   * @Date 2025-08-29 17:52:08
   */
  virtual bool SetNextWrite(uint32_t* data) = 0;

  /**
   * @brief 获取读取事件标志
   * @return [true]：有数据可读，[false]：无数据可读
   * @Date 2025-08-29 17:52:43
   */
  virtual bool GetReadEventFlag() = 0;

  /**
   * @brief 获取写入事件标志
   * @return [true]：可以继续写入数据，[false]：不能写入数据
   * @Date 2025-08-29 17:52:43
   */
  virtual bool GetWriteEventFlag() = 0;

  virtual void Deinit() = 0;
#endif
};

class BusSpiGuide : public Tool {
 public:
  BusSpiGuide() = default;

  virtual bool Init(int32_t freq_hz, int32_t cs) = 0;
  virtual bool Write(const void* data, size_t byte) = 0;
  virtual bool Read(void* data, size_t byte) = 0;
  virtual bool WriteRead(
      const void* write_data, void* read_data, size_t data_byte) = 0;

  bool Read(const uint8_t write_c8, uint8_t* read_d8);
  bool Read(
      const uint8_t write_c8, uint8_t* read_data, size_t read_data_length);
  bool Write(const uint8_t write_c8);
  bool Write(const uint8_t write_c8, const uint8_t write_d8);
  bool Write(const uint8_t write_c8, const uint8_t* write_data,
      size_t write_data_length);

  /**
   * @brief 传输数据结构 [CMD(8 bit) | REG(16 bit) |
   * 0xAA(等待码，有可能有有可能没有，一般为 0xAA) | ReadData(8
   * bit)(要读出的数据) | ReadData(8 bit)(要读出的数据) | ......]
   * @param write_c8 一般为命令位
   * @param write_c16 一般为寄存器地址位
   * @param *read_data 要读出数据的指针
   * @param read_data_length 要读出的数据长度
   * @return
   * @Date 2025-01-17 13:53:33
   */
  bool Read(const uint8_t write_c8, const uint16_t write_c16,
      uint8_t* read_data, size_t read_data_length);

  bool Read(const uint8_t write_c8_1, const uint8_t write_c8_2,
      uint8_t* read_data, size_t read_data_length);

  bool Read(
      const uint8_t write_c8, const uint16_t write_c16, uint8_t* read_data);

  /**
   * @brief 传输数据结构 [CMD(8 bit) | REG(16 bit) | WriteData(8
   * bit)(要写入的数据) | WriteData(8 bit)(要写入的数据) | ......]
   * @param write_c8 一般为命令位
   * @param write_c16 一般为寄存器地址位
   * @param *write_data 要写入数据的指针
   * @param write_data_length 要写入的数据长度
   * @return
   * @Date 2025-01-17 13:48:09
   */
  bool Write(const uint8_t write_c8, const uint16_t write_c16,
      const uint8_t* write_data, size_t write_data_length);

  bool Write(const uint8_t write_c8_1, const uint8_t write_c8_2,
      const uint8_t* write_data, size_t write_data_length);

  bool Write(const uint8_t write_c8, const uint16_t write_c16,
      const uint8_t write_data);
};

class BusQspiGuide : public Tool {
 public:
  BusQspiGuide() = default;

  virtual bool Init(int32_t freq_hz, int32_t cs) = 0;
  virtual bool Write(const void* data, size_t byte,
      uint32_t flags = static_cast<uint32_t>(NULL),
      bool cs_keep_active = false) = 0;
};

class BusUartGuide : public Tool {
 public:
  BusUartGuide() = default;

  virtual bool Init(int32_t baud_rate = CPP_BUS_DRIVER_DEFAULT_VALUE) = 0;

  virtual int32_t Read(void* data, uint32_t length) = 0;
  virtual int32_t Write(const void* data, size_t length) = 0;

  virtual size_t GetRxBufferLength() = 0;
  virtual bool ClearRxBufferData() = 0;
  virtual bool SetBaudRate(uint32_t baud_rate) = 0;
  virtual uint32_t GetBaudRate() = 0;
};

class BusSdioGuide : public Tool {
 public:
  BusSdioGuide() = default;

  virtual bool Init(int32_t freq_hz) = 0;
  virtual bool WaitInterrupt(uint32_t timeout_ms) = 0;
  virtual bool Read(
      uint32_t function, uint32_t write_c32, void* data, size_t byte) = 0;
  virtual bool Read(uint32_t function, uint32_t write_c32, uint8_t* data) = 0;
  virtual bool ReadBlock(
      uint32_t function, uint32_t write_c32, void* data, size_t byte) = 0;
  virtual bool Write(
      uint32_t function, uint32_t write_c32, const void* data, size_t byte) = 0;
  virtual bool Write(uint32_t function, uint32_t write_c32, uint8_t data,
      uint8_t* read_d8_verify = NULL) = 0;
  virtual bool WriteBlock(
      uint32_t function, uint32_t write_c32, const void* data, size_t byte) = 0;
};

class BusMipiGuide : public Tool {
 public:
  BusMipiGuide() = default;

  virtual bool Init(float freq_mhz, float lane_bit_rate_mbps,
      InitSequenceFormat init_sequence_format) = 0;
  virtual bool StartTransmit() = 0;
  virtual bool Read(int32_t cmd, void* data, size_t byte) = 0;
  virtual bool Write(int32_t cmd, const void* data, size_t byte) = 0;
  virtual bool Write(uint16_t x_start, uint16_t x_end, uint16_t y_start,
      uint16_t y_end, const void* data) = 0;

  bool Write(const uint8_t write_c8);
  bool Write(const uint8_t write_c8, const uint8_t write_d8);
};
}  // namespace cpp_bus_driver