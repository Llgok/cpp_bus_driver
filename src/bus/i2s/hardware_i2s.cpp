/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-04-29 16:26:19
 * @License: GPL 3.0
 */
#include "hardware_i2s.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
bool HardwareI2s::Init(i2s_mclk_multiple_t mclk_multiple,
    uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width) {
  if (data_mode_ == DataMode::kInputOutput) {
    if ((chan_tx_handle_ != nullptr) && (chan_rx_handle_ != nullptr)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "HardwareI2s has been initialized\n");
      return true;
    }
  } else {
    if ((chan_tx_handle_ != nullptr) || (chan_rx_handle_ != nullptr)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "HardwareI2s has been initialized\n");
      return true;
    }
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config port_: %d\n", port_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config ws_lrck_: %d\n", ws_lrck_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config bclk_: %d\n", bclk_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config mclk_: %d\n", mclk_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config mclk_multiple: %d\n", mclk_multiple);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config sample_rate_hz: %d hz\n", sample_rate_hz);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config data_bit_width: %d\n", data_bit_width);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config i2s_mode_: %d\n", i2s_mode_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config clock_source_: %d\n", clock_source_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config slot_mode_in_: %d\n", slot_mode_in_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config slot_mode_out_: %d\n", slot_mode_out_);

  i2s_chan_config_t chan_config =
      I2S_CHANNEL_DEFAULT_CONFIG(port_, I2S_ROLE_MASTER);
  // 自动清除DMA缓冲区中的旧数据
  chan_config.auto_clear = true;

  if (data_mode_ == DataMode::kInputOutput) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config data_mode: input_output\n");
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config data_in_: %d\n", data_in_);
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config data_out_: %d\n", data_out_);

    esp_err_t result =
        i2s_new_channel(&chan_config, &chan_tx_handle_, &chan_rx_handle_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "i2s_new_channel failed (error code: %#X)\n", result);
      return false;
    }

    switch (i2s_mode_) {
      case I2sMode::kStd: {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
            "HardwareI2s config i2s_mode: std\n");

        i2s_std_config_t config = {
            .clk_cfg =
                {
                    .sample_rate_hz = sample_rate_hz,
                    .clk_src = clock_source_,
#if SOC_I2S_HW_VERSION_2
                    .ext_clk_freq_hz = 0,
#endif
                    .mclk_multiple = mclk_multiple,
                    .bclk_div = 8,
                },
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                data_bit_width, slot_mode_out_),
            .gpio_cfg =
                {
                    .mclk = static_cast<gpio_num_t>(mclk_),
                    .bclk = static_cast<gpio_num_t>(bclk_),
                    .ws = static_cast<gpio_num_t>(ws_lrck_),
                    .dout = static_cast<gpio_num_t>(data_out_),
                    .din = static_cast<gpio_num_t>(data_in_),
                    .invert_flags =
                        {
                            .mclk_inv = 0,
                            .bclk_inv = 0,
                            .ws_inv = 0,
                        },
                },
        };

        result = i2s_channel_init_std_mode(chan_tx_handle_, &config);
        if (result != ESP_OK) {
          LogMessage(LogLevel::kBus, __FILE__, __LINE__,
              "i2s_channel_init_std_mode failed (error code: %#X)\n", result);
          return false;
        }

        config.slot_cfg.slot_mode = slot_mode_in_;

        result = i2s_channel_init_std_mode(chan_rx_handle_, &config);
        if (result != ESP_OK) {
          LogMessage(LogLevel::kBus, __FILE__, __LINE__,
              "i2s_channel_init_std_mode failed (error code: %#X)\n", result);
          return false;
        }

        break;
      }
      case I2sMode::kPdm: {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
            "HardwareI2s config i2s_mode: pdm\n");

        i2s_pdm_rx_config_t rx_config = {
            .clk_cfg =
                {
                    .sample_rate_hz = sample_rate_hz,
                    .clk_src = clock_source_,
                    .mclk_multiple = mclk_multiple,
                    .dn_sample_mode = I2S_PDM_DSR_8S,
                    .bclk_div = 8,
                },
            .slot_cfg =
                I2S_PDM_RX_SLOT_DEFAULT_CONFIG(data_bit_width, slot_mode_in_),
            .gpio_cfg =
                {
                    .clk = static_cast<gpio_num_t>(ws_lrck_),
                    .din = static_cast<gpio_num_t>(data_in_),
                    .invert_flags =
                        {
                            .clk_inv = false,
                        },
                },
        };

        i2s_pdm_tx_config_t tx_config = {
            .clk_cfg =
                {
                    .sample_rate_hz = sample_rate_hz,
                    .clk_src = clock_source_,
                    .mclk_multiple = mclk_multiple,
                    .up_sample_fp = 960,
                    .up_sample_fs = 480,
                    .bclk_div = 8,
                },
            .slot_cfg =
                I2S_PDM_TX_SLOT_DEFAULT_CONFIG(data_bit_width, slot_mode_out_),
            .gpio_cfg =
                {
                    .clk = static_cast<gpio_num_t>(ws_lrck_),
                    .dout = static_cast<gpio_num_t>(data_out_),
#if SOC_I2S_PDM_MAX_TX_LINES > 1
                    .dout2 = GPIO_NUM_NC,
#endif
                    .invert_flags =
                        {
                            .clk_inv = false,
                        },
                },
        };

        result = i2s_channel_init_pdm_rx_mode(chan_rx_handle_, &rx_config);
        if (result != ESP_OK) {
          LogMessage(LogLevel::kBus, __FILE__, __LINE__,
              "i2s_channel_init_pdm_rx_mode failed (error code: %#X)\n",
              result);
          return false;
        }

        result = i2s_channel_init_pdm_tx_mode(chan_tx_handle_, &tx_config);
        if (result != ESP_OK) {
          LogMessage(LogLevel::kBus, __FILE__, __LINE__,
              "i2s_channel_init_pdm_tx_mode failed (error code: %#X)\n",
              result);
          return false;
        }

        break;
      }
      default:
        break;
    }

    result = i2s_channel_enable(chan_tx_handle_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "i2s_channel_enable failed (error code: %#X)\n", result);
      return false;
    }

    result = i2s_channel_enable(chan_rx_handle_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "i2s_channel_enable failed (error code: %#X)\n", result);
      return false;
    }
  } else {
    switch (i2s_mode_) {
      case I2sMode::kStd: {
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
            "HardwareI2s config i2s_mode: std\n");
        i2s_std_config_t config = {
            .clk_cfg =
                {
                    .sample_rate_hz = sample_rate_hz,
                    .clk_src = clock_source_,
#if SOC_I2S_HW_VERSION_2
                    .ext_clk_freq_hz = 0,
#endif
                    .mclk_multiple = mclk_multiple,
                    .bclk_div = 8,
                },
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                data_bit_width, slot_mode_out_),
            .gpio_cfg =
                {
                    .mclk = static_cast<gpio_num_t>(mclk_),
                    .bclk = static_cast<gpio_num_t>(bclk_),
                    .ws = static_cast<gpio_num_t>(ws_lrck_),
                    .dout = I2S_GPIO_UNUSED,
                    .din = I2S_GPIO_UNUSED,
                    .invert_flags =
                        {
                            .mclk_inv = 0,
                            .bclk_inv = 0,
                            .ws_inv = 0,
                        },
                },
        };

        switch (data_mode_) {
          case DataMode::kInput: {
            LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
                "HardwareI2s config data_mode: input\n");

            config.gpio_cfg.din = static_cast<gpio_num_t>(data_in_);
            config.slot_cfg.slot_mode = slot_mode_in_;

            esp_err_t result =
                i2s_new_channel(&chan_config, NULL, &chan_rx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_new_channel failed (error code: %#X)\n", result);
              return false;
            }

            result = i2s_channel_init_std_mode(chan_rx_handle_, &config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_init_std_mode failed (error code: %#X)\n",
                  result);
              return false;
            }

            result = i2s_channel_enable(chan_rx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_enable failed (error code: %#X)\n", result);
              return false;
            }
          } break;
          case DataMode::kOutput: {
            LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
                "HardwareI2s config data_mode: output\n");

            config.gpio_cfg.dout = static_cast<gpio_num_t>(data_out_);

            esp_err_t result =
                i2s_new_channel(&chan_config, &chan_tx_handle_, NULL);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_new_channel failed (error code: %#X)\n", result);
              return false;
            }

            result = i2s_channel_init_std_mode(chan_tx_handle_, &config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_init_std_mode failed (error code: %#X)\n",
                  result);
              return false;
            }

            result = i2s_channel_enable(chan_tx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_enable failed (error code: %#X)\n", result);
              return false;
            }
          } break;

          default:
            break;
        }

        break;
      }
      case I2sMode::kPdm:
        LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
            "HardwareI2s config i2s_mode: pdm\n");

        switch (data_mode_) {
          case DataMode::kInput: {
            LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
                "HardwareI2s config data_mode: input\n");
            LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
                "HardwareI2s config data_in_: %d\n", data_in_);

            i2s_pdm_rx_config_t rx_config = {
                .clk_cfg =
                    {
                        .sample_rate_hz = sample_rate_hz,
                        .clk_src = clock_source_,
                        .mclk_multiple = mclk_multiple,
                        .dn_sample_mode = I2S_PDM_DSR_8S,
                        .bclk_div = 8,
                    },
                .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(
                    data_bit_width, slot_mode_in_),
                .gpio_cfg =
                    {
                        .clk = static_cast<gpio_num_t>(ws_lrck_),
                        .din = static_cast<gpio_num_t>(data_in_),
                        .invert_flags =
                            {
                                .clk_inv = false,
                            },
                    },
            };

            esp_err_t result =
                i2s_new_channel(&chan_config, NULL, &chan_rx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_new_channel failed (error code: %#X)\n", result);
              return false;
            }

            result = i2s_channel_init_pdm_rx_mode(chan_rx_handle_, &rx_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_init_pdm_rx_mode failed (error code: %#X)\n",
                  result);
              return false;
            }

            result = i2s_channel_enable(chan_rx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_enable failed (error code: %#X)\n", result);
              return false;
            }

            break;
          }
          case DataMode::kOutput: {
            LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
                "HardwareI2s config data_mode: output\n");
            LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
                "HardwareI2s config data_out_: %d\n", data_out_);

            i2s_pdm_tx_config_t tx_config = {
                .clk_cfg =
                    {
                        .sample_rate_hz = sample_rate_hz,
                        .clk_src = clock_source_,
                        .mclk_multiple = mclk_multiple,
                        .up_sample_fp = 960,
                        .up_sample_fs = 480,
                        .bclk_div = 8,
                    },
                .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(
                    data_bit_width, slot_mode_out_),
                .gpio_cfg =
                    {
                        .clk = static_cast<gpio_num_t>(ws_lrck_),
                        .dout = static_cast<gpio_num_t>(data_out_),
#if SOC_I2S_PDM_MAX_TX_LINES > 1
                        .dout2 = GPIO_NUM_NC,
#endif
                        .invert_flags =
                            {
                                .clk_inv = false,
                            },
                    },
            };

            esp_err_t result =
                i2s_new_channel(&chan_config, &chan_tx_handle_, NULL);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_new_channel failed (error code: %#X)\n", result);
              return false;
            }

            result = i2s_channel_init_pdm_tx_mode(chan_tx_handle_, &tx_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_init_pdm_tx_mode failed (error code: %#X)\n",
                  result);
              return false;
            }

            result = i2s_channel_enable(chan_tx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_enable failed (error code: %#X)\n", result);
              return false;
            }

            break;
          }
          default:
            break;
        }
        break;

      default:
        break;
    }
  }

  mclk_multiple_ = mclk_multiple;
  sample_rate_hz_ = sample_rate_hz;
  data_bit_width_ = data_bit_width;

  return true;
}

size_t HardwareI2s::Read(void* data, size_t byte) {
  if (chan_rx_handle_ == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  size_t buffer = 0;
  esp_err_t result = i2s_channel_read(chan_rx_handle_, data, byte, &buffer,
      CPP_BUS_DRIVER_DEFAULT_I2S_WAIT_TIMEOUT_MS);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2s_channel_read failed (error code: %#X)\n", result);
    return false;
  }

  return buffer;
}

size_t HardwareI2s::Write(const void* data, size_t byte) {
  if (chan_tx_handle_ == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  size_t buffer = 0;
  esp_err_t result = i2s_channel_write(chan_tx_handle_, data, byte, &buffer,
      CPP_BUS_DRIVER_DEFAULT_I2S_WAIT_TIMEOUT_MS);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "i2s_channel_write failed (error code: %#X)\n", result);
    return false;
  }

  return buffer;
}

bool HardwareI2s::SetClockReconfig(i2s_mclk_multiple_t mclk_multiple,
    uint32_t sample_rate_hz, DataMode data_mode) {
  if (data_mode_ == DataMode::kInputOutput) {
    switch (data_mode) {
      case DataMode::kInput:
        switch (i2s_mode_) {
          case I2sMode::kStd: {
            i2s_std_clk_config_t clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
#if SOC_I2S_HW_VERSION_2
                .ext_clk_freq_hz = 0,
#endif
                .mclk_multiple = mclk_multiple,
                .bclk_div = 8,
            };

            esp_err_t result =
                i2s_channel_reconfig_std_clock(chan_rx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }

            break;
          }
          case I2sMode::kPdm: {
            i2s_pdm_rx_clk_config_t rx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .dn_sample_mode = I2S_PDM_DSR_8S,
                .bclk_div = 8,
            };

            esp_err_t result = i2s_channel_reconfig_pdm_rx_clock(
                chan_rx_handle_, &rx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_rx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
          }
          default:
            break;
        }

        break;
      case DataMode::kOutput:
        switch (i2s_mode_) {
          case I2sMode::kStd: {
            i2s_std_clk_config_t clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
#if SOC_I2S_HW_VERSION_2
                .ext_clk_freq_hz = 0,
#endif
                .mclk_multiple = mclk_multiple,
                .bclk_div = 8,
            };

            esp_err_t result =
                i2s_channel_reconfig_std_clock(chan_tx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }

            break;
          }
          case I2sMode::kPdm: {
            i2s_pdm_tx_clk_config_t tx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .up_sample_fp = 960,
                .up_sample_fs = 480,
                .bclk_div = 8,
            };

            esp_err_t result = i2s_channel_reconfig_pdm_tx_clock(
                chan_tx_handle_, &tx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_tx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
          }
          default:
            break;
        }

        break;
      case DataMode::kInputOutput:
        switch (i2s_mode_) {
          case I2sMode::kStd: {
            i2s_std_clk_config_t clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
#if SOC_I2S_HW_VERSION_2
                .ext_clk_freq_hz = 0,
#endif
                .mclk_multiple = mclk_multiple,
                .bclk_div = 8,
            };

            esp_err_t result =
                i2s_channel_reconfig_std_clock(chan_tx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }

            result =
                i2s_channel_reconfig_std_clock(chan_rx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }

            break;
          }
          case I2sMode::kPdm: {
            i2s_pdm_rx_clk_config_t rx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .dn_sample_mode = I2S_PDM_DSR_8S,
                .bclk_div = 8,
            };

            i2s_pdm_tx_clk_config_t tx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .up_sample_fp = 960,
                .up_sample_fs = 480,
                .bclk_div = 8,
            };

            esp_err_t result = i2s_channel_reconfig_pdm_rx_clock(
                chan_rx_handle_, &rx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_rx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            result = i2s_channel_reconfig_pdm_tx_clock(
                chan_tx_handle_, &tx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_tx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
          }
          default:
            break;
        }

        break;

      default:
        break;
    }
  } else {
    switch (i2s_mode_) {
      case I2sMode::kStd: {
        i2s_std_clk_config_t clk_config = {
            .sample_rate_hz = sample_rate_hz,
            .clk_src = clock_source_,
#if SOC_I2S_HW_VERSION_2
            .ext_clk_freq_hz = 0,
#endif
            .mclk_multiple = mclk_multiple,
            .bclk_div = 8,
        };

        switch (data_mode_) {
          case DataMode::kInput: {
            esp_err_t result =
                i2s_channel_reconfig_std_clock(chan_rx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }
          } break;
          case DataMode::kOutput: {
            esp_err_t result =
                i2s_channel_reconfig_std_clock(chan_tx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }
          } break;

          default:
            break;
        }

        break;
      }
      case I2sMode::kPdm:
        switch (data_mode_) {
          case DataMode::kInput: {
            i2s_pdm_rx_clk_config_t rx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .dn_sample_mode = I2S_PDM_DSR_8S,
                .bclk_div = 8,
            };

            esp_err_t result = i2s_channel_reconfig_pdm_rx_clock(
                chan_rx_handle_, &rx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_rx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
          }
          case DataMode::kOutput: {
            i2s_pdm_tx_clk_config_t tx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .up_sample_fp = 960,
                .up_sample_fs = 480,
                .bclk_div = 8,
            };

            esp_err_t result = i2s_channel_reconfig_pdm_tx_clock(
                chan_tx_handle_, &tx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_tx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
          }
          default:
            break;
        }
        break;

      default:
        break;
    }
  }

  return true;
}

bool HardwareI2s::SetChannelEnable(bool enable, DataMode data_mode) {
  if (enable) {
    if (data_mode_ == DataMode::kInputOutput) {
      switch (data_mode) {
        case DataMode::kInput: {
          esp_err_t result = i2s_channel_enable(chan_rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }

        case DataMode::kOutput: {
          esp_err_t result = i2s_channel_enable(chan_tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kInputOutput: {
          esp_err_t result = i2s_channel_enable(chan_tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }

          result = i2s_channel_enable(chan_rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }

        default:
          break;
      }
    } else {
      switch (data_mode_) {
        case DataMode::kInput: {
          esp_err_t result = i2s_channel_enable(chan_rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kOutput: {
          esp_err_t result = i2s_channel_enable(chan_tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        default:
          break;
      }
    }
  } else {
    if (data_mode_ == DataMode::kInputOutput) {
      switch (data_mode) {
        case DataMode::kInput: {
          esp_err_t result = i2s_channel_disable(chan_rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kOutput: {
          esp_err_t result = i2s_channel_disable(chan_tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kInputOutput: {
          esp_err_t result = i2s_channel_disable(chan_tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }

          result = i2s_channel_disable(chan_rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        default:
          break;
      }
    } else {
      switch (data_mode_) {
        case DataMode::kInput: {
          esp_err_t result = i2s_channel_disable(chan_rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kOutput: {
          esp_err_t result = i2s_channel_disable(chan_tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kBus, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        default:
          break;
      }
    }
  }

  return true;
}

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
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
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_32_X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 32.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 32\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_48_X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 48.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 48\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_64_X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 64.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 64\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_96_X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 96.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 96\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_128_X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 128.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 128\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_192_X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 192.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 192\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_256_X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 256.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 256\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_384_X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 384.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 384\n");
      break;
    case nrf_i2s_ratio_t::NRF_I2S_RATIO_512_X:
      buffer_mclk_freq_mhz =
          (static_cast<double>(sample_rate_hz) * 512.0) / 1000000.0;
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "HardwareI2s config mclk_multiple: 512\n");
      break;

    default:
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
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
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_125;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 125 (division_freq: %.6f mhz)\n",
        buffer_division_freq_125);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_63_42) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_63;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 63 (division_freq: %.6f mhz)\n",
        buffer_division_freq_63);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_42_32) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_42;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 42 (division_freq: %.6f mhz)\n",
        buffer_division_freq_42);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_32_31) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_32;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 32 (division_freq: %.6f mhz)\n",
        buffer_division_freq_32);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_31_30) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_31;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 31 (division_freq: %.6f mhz)\n",
        buffer_division_freq_31);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_30_23) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_30;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 30 (division_freq: %.6f mhz)\n",
        buffer_division_freq_30);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_23_21) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_23;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 23 (division_freq: %.6f mhz)\n",
        buffer_division_freq_23);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_21_16) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_21;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 21 (division_freq: %.6f mhz)\n",
        buffer_division_freq_21);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_16_15) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_16;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 16 (division_freq: %.6f mhz)\n",
        buffer_division_freq_16);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_15_11) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_15;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 15 (division_freq: %.6f mhz)\n",
        buffer_division_freq_15);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_11_10) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_11;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 11 (division_freq: %.6f mhz)\n",
        buffer_division_freq_11);
  } else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_10_8) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_10;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 10 (division_freq: %.6f mhz)\n",
        buffer_division_freq_10);
  } else if (buffer_mclk_freq_mhz <= buffer_division_freq_8) {
    buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32_MDIV_8;
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareI2s config mclk_division: 8 (division_freq: %.6f mhz)\n",
        buffer_division_freq_8);
  } else {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
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
        LogLevel::kBus, __FILE__, __LINE__, "nrf_i2s_configure failed\n");
    return false;
  }

  return true;
}

bool HardwareI2s::StartTransmit(
    uint32_t* write_data, uint32_t* read_data, size_t max_data_length) {
  if (write_data == nullptr && read_data == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (max_data_length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  if (write_data != nullptr) {
    if (!nrfx_is_in_ram(write_data)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "nrfx_is_in_ram failed (write_data is not located in the data "
          "ram region)\n");
      return false;
    }
    if (!nrfx_is_word_aligned(write_data)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "nrfx_is_word_aligned failed (write_data is not aligned to a "
          "32-bit word)\n");
      return false;
    }

    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
  }
  if (read_data != nullptr) {
    if (!nrfx_is_in_ram(read_data)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "nrfx_is_in_ram failed (write_data is not located in the data "
          "ram region)\n");
      return false;
    }
    if (!nrfx_is_word_aligned(read_data)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
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
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!nrfx_is_in_ram(data)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "nrfx_is_in_ram failed (data is not located in the data ram region)\n");
    return false;
  }
  if (!nrfx_is_word_aligned(data)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "nrfx_is_word_aligned failed (data is not aligned to a 32-bit word)\n");
    return false;
  }

  nrf_i2s_rx_buffer_set(NRF_I2S, data);

  nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);

  return true;
}

bool HardwareI2s::SetNextWrite(uint32_t* data) {
  if (data == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!nrfx_is_in_ram(data)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "nrfx_is_in_ram failed (data is not located in the data ram region)\n");
    return false;
  }
  if (!nrfx_is_word_aligned(data)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
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

#endif

bool HardwareI2s::Deinit() {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  bool result = true;

  if (chan_tx_handle_ != nullptr) {
    esp_err_t ret = i2s_channel_disable(chan_tx_handle_);
    if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "i2s_channel_disable failed (error code: %#X)\n", ret);
      result = false;
    }

    ret = i2s_del_channel(chan_tx_handle_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "i2s_del_channel failed (error code: %#X)\n", ret);
      result = false;
    } else {
      chan_tx_handle_ = nullptr;
    }
  }

  if (chan_rx_handle_ != nullptr) {
    esp_err_t ret = i2s_channel_disable(chan_rx_handle_);
    if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "i2s_channel_disable failed (error code: %#X)\n", ret);
      result = false;
    }

    ret = i2s_del_channel(chan_rx_handle_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "i2s_del_channel failed (error code: %#X)\n", ret);
      result = false;
    } else {
      chan_rx_handle_ = nullptr;
    }
  }

  return result;

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
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

#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
  return false;

#endif
}

}  // namespace cpp_bus_driver
