
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-04-29 16:17:16
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace cpp_bus_driver {
class HardwareI2s final : public BusI2sGuide {
 public:
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  enum class I2sMode {
    kStd,  // 标准模式
    kPdm,  // pdm模式
  };

  // 配置输入和输出设备
  explicit HardwareI2s(int32_t data_in, int32_t data_out, int32_t ws_lrck,
      int32_t bclk, int32_t mclk, i2s_port_t port = I2S_NUM_0,
      DataMode data_mode = DataMode::kInputOutput,
      I2sMode i2s_mode = I2sMode::kStd,
      i2s_clock_src_t clock_source = I2S_CLK_SRC_DEFAULT,
      i2s_slot_mode_t slot_mode_in = I2S_SLOT_MODE_STEREO,
      i2s_slot_mode_t slot_mode_out = I2S_SLOT_MODE_STEREO)
      : data_in_(data_in),
        data_out_(data_out),
        ws_lrck_(ws_lrck),
        bclk_(bclk),
        mclk_(mclk),
        port_(port),
        data_mode_(data_mode),
        i2s_mode_(i2s_mode),
        clock_source_(clock_source),
        slot_mode_in_(slot_mode_in),
        slot_mode_out_(slot_mode_out) {}

  bool Init(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz,
      i2s_data_bit_width_t data_bit_width) override;

  size_t Read(void* data, size_t byte) override;
  size_t Write(const void* data, size_t byte) override;

  bool SetClockReconfig(i2s_mclk_multiple_t mclk_multiple,
      uint32_t sample_rate_hz,
      DataMode data_mode = DataMode::kInputOutput) override;
  bool SetChannelEnable(
      bool enable, DataMode data_mode = DataMode::kInputOutput) override;

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  // 配置输入和输出设备
  HardwareI2s(int32_t data_in, int32_t data_out, int32_t ws_lrck, int32_t bclk,
      int32_t mclk,
      nrf_i2s_channels_t channel = nrf_i2s_channels_t::NRF_I2S_CHANNELS_STEREO)
      : data_in_(data_in),
        data_out_(data_out),
        ws_lrck_(ws_lrck),
        bclk_(bclk),
        mclk_(mclk),
        channel_(channel) {}

  bool Init(nrf_i2s_ratio_t mclk_multiple, uint32_t sample_rate_hz,
      nrf_i2s_swidth_t data_bit_width) override;

  bool StartTransmit(uint32_t* write_data, uint32_t* read_data,
      size_t max_data_length) override;
  void StopTransmit() override;
  bool SetNextRead(uint32_t* data) override;
  bool SetNextWrite(uint32_t* data) override;
  bool GetReadEventFlag() override;
  bool GetWriteEventFlag() override;
#endif

  bool Deinit() override;

 private:
  int32_t data_in_, data_out_;
  int32_t ws_lrck_, bclk_, mclk_;

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  i2s_port_t port_;
  i2s_chan_handle_t chan_tx_handle_ = nullptr;
  i2s_chan_handle_t chan_rx_handle_ = nullptr;
#endif

  uint16_t mclk_multiple_ = CPP_BUS_DRIVER_DEFAULT_VALUE;
  uint32_t sample_rate_hz_ = CPP_BUS_DRIVER_DEFAULT_VALUE;
  uint8_t data_bit_width_ = CPP_BUS_DRIVER_DEFAULT_VALUE;

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  DataMode data_mode_;
  I2sMode i2s_mode_;
  i2s_clock_src_t clock_source_;
  i2s_slot_mode_t slot_mode_in_;
  i2s_slot_mode_t slot_mode_out_;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  nrf_i2s_channels_t channel_;
#endif
};
}  // namespace cpp_bus_driver
