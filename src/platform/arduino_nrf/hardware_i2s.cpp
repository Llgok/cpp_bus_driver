/*
 * @Description: 跨平台硬件 I2S 音频总线驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-09-05 14:57:21
 * @License: GPL 3.0
 */
#include "bus/i2s/hardware_i2s.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52

#include "Arduino.h"
#include "nrf_gpio.h"
#include "nrfx.h"

namespace cpp_bus_driver {
bool HardwareI2s::Init(nrf_i2s_ratio_t mclk_multiple, uint32_t sample_rate_hz,
    nrf_i2s_swidth_t data_bit_width) {
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config ws_lrck_: %d\n", ws_lrck_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config bclk_: %d\n", bclk_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config mclk_: %d\n", mclk_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config channel_: %d\n", channel_);

  nrf_i2s_mck_t buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_DISABLED;
  double buffer_mclk_freq_mhz = 0.0;

  switch (mclk_multiple) {
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_32X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 32.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 32\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_48X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 48.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 48\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_64X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 64.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 64\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_96X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 96.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 96\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_128X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 128.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 128\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_192X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 192.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 192\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_256X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 256.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 256\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_384X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 384.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 384\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_512X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 512.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 512\n");
      break;

    default:
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "HardwareI2s mclk_multiple check failed (unknown mclk_multiple)\n");
      return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config sample_rate_hz: %d hz\n", sample_rate_hz);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config mclk_freq_mhz: %.6f mhz\n", buffer_mclk_freq_mhz);

  // 定义每个分频对应的频率值
  constexpr const double buffer_division_freq_125 = 32.0 / 125.0;
  constexpr const double buffer_division_freq_63 = 32.0 / 63.0;
  constexpr const double buffer_division_freq_42 = 32.0 / 42.0;
  constexpr const double buffer_division_freq_32 = 32.0 / 32.0;
  constexpr const double buffer_division_freq_31 = 32.0 / 31.0;
  constexpr const double buffer_division_freq_30 = 32.0 / 30.0;
  constexpr const double buffer_division_freq_23 = 32.0 / 23.0;
  constexpr const double buffer_division_freq_21 = 32.0 / 21.0;
  constexpr const double buffer_division_freq_16 = 32.0 / 16.0;
  constexpr const double buffer_division_freq_15 = 32.0 / 15.0;
  constexpr const double buffer_division_freq_11 = 32.0 / 11.0;
  constexpr const double buffer_division_freq_10 = 32.0 / 10.0;
  constexpr const double buffer_division_freq_8 =
      32.0 / 8.0 + 1;  // 计算出来的值有可能大于32.0 / 8.0

  // 计算每个范围的中点
  constexpr const double buffer_division_freq_mid_125_63 =
      (buffer_division_freq_125 + buffer_division_freq_63) / 2.0;
  constexpr const double buffer_division_freq_mid_63_42 =
      (buffer_division_freq_63 + buffer_division_freq_42) / 2.0;
  constexpr const double buffer_division_freq_mid_42_32 =
      (buffer_division_freq_42 + buffer_division_freq_32) / 2.0;
  constexpr const double buffer_division_freq_mid_32_31 =
      (buffer_division_freq_32 + buffer_division_freq_31) / 2.0;
  constexpr const double buffer_division_freq_mid_31_30 =
      (buffer_division_freq_31 + buffer_division_freq_30) / 2.0;
  constexpr const double buffer_division_freq_mid_30_23 =
      (buffer_division_freq_30 + buffer_division_freq_23) / 2.0;
  constexpr const double buffer_division_freq_mid_23_21 =
      (buffer_division_freq_23 + buffer_division_freq_21) / 2.0;
  constexpr const double buffer_division_freq_mid_21_16 =
      (buffer_division_freq_21 + buffer_division_freq_16) / 2.0;
  constexpr const double buffer_division_freq_mid_16_15 =
      (buffer_division_freq_16 + buffer_division_freq_15) / 2.0;
  constexpr const double buffer_division_freq_mid_15_11 =
      (buffer_division_freq_15 + buffer_division_freq_11) / 2.0;
  constexpr const double buffer_division_freq_mid_11_10 =
      (buffer_division_freq_11 + buffer_division_freq_10) / 2.0;
  constexpr const double buffer_division_freq_mid_10_8 =
      (buffer_division_freq_10 + buffer_division_freq_8) / 2.0;

  if ((buffer_mclk_freq_mhz >= buffer_division_freq_125) &&
      (buffer_mclk_freq_mhz < buffer_division_freq_mid_125_63)) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV125;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 125 (division_freq: %.6f mhz)\n",
        buffer_division_freq_125);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_63_42) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV63;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 63 (division_freq: %.6f mhz)\n",
        buffer_division_freq_63);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_42_32) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV42;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 42 (division_freq: %.6f mhz)\n",
        buffer_division_freq_42);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_32_31) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV32;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 32 (division_freq: %.6f mhz)\n",
        buffer_division_freq_32);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_31_30) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV31;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 31 (division_freq: %.6f mhz)\n",
        buffer_division_freq_31);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_30_23) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV30;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 30 (division_freq: %.6f mhz)\n",
        buffer_division_freq_30);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_23_21) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV23;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 23 (division_freq: %.6f mhz)\n",
        buffer_division_freq_23);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_21_16) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV21;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 21 (division_freq: %.6f mhz)\n",
        buffer_division_freq_21);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_16_15) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV16;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 16 (division_freq: %.6f mhz)\n",
        buffer_division_freq_16);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_15_11) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV15;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 15 (division_freq: %.6f mhz)\n",
        buffer_division_freq_15);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_11_10) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV11;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 11 (division_freq: %.6f mhz)\n",
        buffer_division_freq_11);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_10_8) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV10;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 10 (division_freq: %.6f mhz)\n",
        buffer_division_freq_10);
  } else if (buffer_mclk_freq_mhz <= buffer_division_freq_8) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV8;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 8 (division_freq: %.6f mhz)\n",
        buffer_division_freq_8);
  } else {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HardwareI2s mclk_division check failed (mclk_division out of "
        "bounds)\n");
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config data_bit_width: %d\n", (data_bit_width + 1) * 8);

  nrf_gpio_cfg_output(bclk_);
  nrf_gpio_cfg_output(ws_lrck_);
  nrf_gpio_cfg_output(mclk_);
  nrf_gpio_cfg_output(data_out_);
  nrf_gpio_cfg_input(data_in_, NRF_GPIO_PIN_NOPULL);
  nrf_i2s_pins_set(NRF_I2S, bclk_, ws_lrck_, mclk_, data_out_, data_in_);

  if (!nrf_i2s_configure(NRF_I2S, nrf_i2s_mode_t::NRF_I2S_MODE_MASTER,
          nrf_i2s_format_t::NRF_I2S_FORMAT_I2S,
          nrf_i2s_align_t::NRF_I2S_ALIGN_LEFT, data_bit_width, channel_,
          buffer_mclk_division, mclk_multiple)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "nrf_i2s_configure failed\n");
    return false;
  }

  return true;
}

bool HardwareI2s::StartTransmit(
    uint32_t* write_data, uint32_t* read_data, size_t max_data_length) {
  if (write_data == nullptr && read_data == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (max_data_length == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  if (write_data != nullptr) {
    if (!nrfx_is_in_ram(write_data)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "nrfx_is_in_ram failed (write_data is not located in the data "
          "ram region)\n");
      return false;
    }
    if (!nrfx_is_word_aligned(write_data)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "nrfx_is_word_aligned failed (write_data is not aligned to a "
          "32-bit word)\n");
      return false;
    }

    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
  }
  if (read_data != nullptr) {
    if (!nrfx_is_in_ram(read_data)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "nrfx_is_in_ram failed (write_data is not located in the data "
          "ram region)\n");
      return false;
    }
    if (!nrfx_is_word_aligned(read_data)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "nrfx_is_word_aligned failed (write_data is not aligned to a "
          "32-bit word)\n");
      return false;
    }

    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
  }

  nrf_i2s_transfer_set(NRF_I2S, max_data_length, read_data, write_data);

  // 启动i2s音频流传输任务
  nrf_i2s_enable(NRF_I2S);

  nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_START);

  return true;
}

void HardwareI2s::StopTransmit() {
  nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_STOP);

  nrf_i2s_disable(NRF_I2S);
}

bool HardwareI2s::SetNextRead(uint32_t* data) {
  if (data == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!nrfx_is_in_ram(data)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "nrfx_is_in_ram failed (data is not located in the data ram region)\n");
    return false;
  }
  if (!nrfx_is_word_aligned(data)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "nrfx_is_word_aligned failed (data is not aligned to a 32-bit word)\n");
    return false;
  }

  nrf_i2s_rx_buffer_set(NRF_I2S, data);

  nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);

  return true;
}

bool HardwareI2s::SetNextWrite(uint32_t* data) {
  if (data == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!nrfx_is_in_ram(data)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "nrfx_is_in_ram failed (data is not located in the data ram region)\n");
    return false;
  }
  if (!nrfx_is_word_aligned(data)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "nrfx_is_word_aligned failed (data is not aligned to a 32-bit word)\n");
    return false;
  }

  nrf_i2s_tx_buffer_set(NRF_I2S, data);

  nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);

  return true;
}

bool HardwareI2s::GetReadEventFlag() {
  return nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
}

bool HardwareI2s::GetWriteEventFlag() {
  return nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
}

bool HardwareI2s::Deinit() {
  StopTransmit();

  nrf_i2s_pins_set(NRF_I2S, NRF_I2S_PIN_NOT_CONNECTED,
      NRF_I2S_PIN_NOT_CONNECTED, NRF_I2S_PIN_NOT_CONNECTED,
      NRF_I2S_PIN_NOT_CONNECTED, NRF_I2S_PIN_NOT_CONNECTED);

  nrf_gpio_cfg_default(bclk_);
  nrf_gpio_cfg_default(ws_lrck_);
  nrf_gpio_cfg_default(mclk_);
  nrf_gpio_cfg_default(data_out_);
  nrf_gpio_cfg_default(data_in_);

  return true;
}

}  // namespace cpp_bus_driver

#endif
