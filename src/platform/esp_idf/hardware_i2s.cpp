/*
 * @Description: ESP-IDF 后端硬件 I2S 总线驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-09-04 10:14:25
 * @LastEditTime: 2026-09-05 14:57:30
 * @License: GPL 3.0
 */
#include "bus/i2s/hardware_i2s.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
#if SOC_I2S_SUPPORTED

#include "driver/gpio.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"
#include "soc/soc_caps.h"

namespace cpp_bus_driver {
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
bool HardwareI2s::Init(i2s_mclk_multiple_t mclk_multiple,
    uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width) {
  if (data_mode_ == DataMode::kInputOutput) {
    if ((tx_handle_ != nullptr) && (rx_handle_ != nullptr)) {
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "HardwareI2s has been initialized\n");
      return true;
    }
  } else {
    if ((tx_handle_ != nullptr) || (rx_handle_ != nullptr)) {
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
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
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config slot_mask_in_: %#X\n", slot_mask_in_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareI2s config slot_mask_out_: %#X\n", slot_mask_out_);

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

    esp_err_t result = i2s_new_channel(&chan_config, &tx_handle_, &rx_handle_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2s_new_channel failed (error code: %#X)\n", result);
      Deinit();
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
        config.slot_cfg.slot_mask = slot_mask_out_;

        result = i2s_channel_init_std_mode(tx_handle_, &config);
        if (result != ESP_OK) {
          LogMessage(LogLevel::kError, __FILE__, __LINE__,
              "i2s_channel_init_std_mode failed (error code: %#X)\n", result);
          Deinit();
          return false;
        }

        config.slot_cfg.slot_mode = slot_mode_in_;
        config.slot_cfg.slot_mask = slot_mask_in_;

        result = i2s_channel_init_std_mode(rx_handle_, &config);
        if (result != ESP_OK) {
          LogMessage(LogLevel::kError, __FILE__, __LINE__,
              "i2s_channel_init_std_mode failed (error code: %#X)\n", result);
          Deinit();
          return false;
        }

        break;
      }
      case I2sMode::kPdm: {
#if SOC_I2S_SUPPORTS_PDM_RX && SOC_I2S_SUPPORTS_PDM_TX
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

        result = i2s_channel_init_pdm_rx_mode(rx_handle_, &rx_config);
        if (result != ESP_OK) {
          LogMessage(LogLevel::kError, __FILE__, __LINE__,
              "i2s_channel_init_pdm_rx_mode failed (error code: %#X)\n",
              result);
          Deinit();
          return false;
        }

        result = i2s_channel_init_pdm_tx_mode(tx_handle_, &tx_config);
        if (result != ESP_OK) {
          LogMessage(LogLevel::kError, __FILE__, __LINE__,
              "i2s_channel_init_pdm_tx_mode failed (error code: %#X)\n",
              result);
          Deinit();
          return false;
        }

        break;
#else
        LogMessage(LogLevel::kError, __FILE__, __LINE__,
            "Requested I2S PDM direction is not supported by this chip\n");
        Deinit();
        return false;
#endif
      }
      default:
        break;
    }

    result = i2s_channel_enable(tx_handle_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2s_channel_enable failed (error code: %#X)\n", result);
      Deinit();
      return false;
    }

    result = i2s_channel_enable(rx_handle_);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2s_channel_enable failed (error code: %#X)\n", result);
      Deinit();
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
        config.slot_cfg.slot_mask = slot_mask_out_;

        switch (data_mode_) {
          case DataMode::kInput: {
            LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
                "HardwareI2s config data_mode: input\n");

            config.gpio_cfg.din = static_cast<gpio_num_t>(data_in_);
            config.slot_cfg.slot_mode = slot_mode_in_;
            config.slot_cfg.slot_mask = slot_mask_in_;

            esp_err_t result =
                i2s_new_channel(&chan_config, nullptr, &rx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_new_channel failed (error code: %#X)\n", result);
              Deinit();
              return false;
            }

            result = i2s_channel_init_std_mode(rx_handle_, &config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_init_std_mode failed (error code: %#X)\n",
                  result);
              Deinit();
              return false;
            }

            result = i2s_channel_enable(rx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_enable failed (error code: %#X)\n", result);
              Deinit();
              return false;
            }
          } break;
          case DataMode::kOutput: {
            LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
                "HardwareI2s config data_mode: output\n");

            config.gpio_cfg.dout = static_cast<gpio_num_t>(data_out_);

            esp_err_t result =
                i2s_new_channel(&chan_config, &tx_handle_, nullptr);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_new_channel failed (error code: %#X)\n", result);
              Deinit();
              return false;
            }

            result = i2s_channel_init_std_mode(tx_handle_, &config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_init_std_mode failed (error code: %#X)\n",
                  result);
              Deinit();
              return false;
            }

            result = i2s_channel_enable(tx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_enable failed (error code: %#X)\n", result);
              Deinit();
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
#if SOC_I2S_SUPPORTS_PDM_RX
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
                i2s_new_channel(&chan_config, nullptr, &rx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_new_channel failed (error code: %#X)\n", result);
              Deinit();
              return false;
            }

            result = i2s_channel_init_pdm_rx_mode(rx_handle_, &rx_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_init_pdm_rx_mode failed (error code: %#X)\n",
                  result);
              Deinit();
              return false;
            }

            result = i2s_channel_enable(rx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_enable failed (error code: %#X)\n", result);
              Deinit();
              return false;
            }

            break;
#else
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "Requested I2S PDM direction is not supported by this chip\n");
            Deinit();
            return false;
#endif
          }
          case DataMode::kOutput: {
#if SOC_I2S_SUPPORTS_PDM_TX
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
                i2s_new_channel(&chan_config, &tx_handle_, nullptr);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_new_channel failed (error code: %#X)\n", result);
              Deinit();
              return false;
            }

            result = i2s_channel_init_pdm_tx_mode(tx_handle_, &tx_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_init_pdm_tx_mode failed (error code: %#X)\n",
                  result);
              Deinit();
              return false;
            }

            result = i2s_channel_enable(tx_handle_);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_enable failed (error code: %#X)\n", result);
              Deinit();
              return false;
            }

            break;
#else
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "Requested I2S PDM direction is not supported by this chip\n");
            Deinit();
            return false;
#endif
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

size_t HardwareI2s::Read(void* data, size_t byte) {
  size_t buffer = 0;
  esp_err_t result =
      i2s_channel_read(rx_handle_, data, byte, &buffer, kDefaultWaitTimeoutMs);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2s_channel_read failed (error code: %#X)\n", result);
    return false;
  }

  return buffer;
}

size_t HardwareI2s::Write(const void* data, size_t byte) {
  size_t buffer = 0;
  esp_err_t result =
      i2s_channel_write(tx_handle_, data, byte, &buffer, kDefaultWaitTimeoutMs);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "i2s_channel_write failed (error code: %#X)\n", result);
    return false;
  }

  return buffer;
}

bool HardwareI2s::ReconfigureClock(i2s_mclk_multiple_t mclk_multiple,
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
                i2s_channel_reconfig_std_clock(rx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }

            break;
          }
          case I2sMode::kPdm: {
#if SOC_I2S_SUPPORTS_PDM_RX
            i2s_pdm_rx_clk_config_t rx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .dn_sample_mode = I2S_PDM_DSR_8S,
                .bclk_div = 8,
            };

            esp_err_t result =
                i2s_channel_reconfig_pdm_rx_clock(rx_handle_, &rx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_rx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
#else
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "Requested I2S PDM direction is not supported by this chip\n");
            return false;
#endif
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
                i2s_channel_reconfig_std_clock(tx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }

            break;
          }
          case I2sMode::kPdm: {
#if SOC_I2S_SUPPORTS_PDM_TX
            i2s_pdm_tx_clk_config_t tx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .up_sample_fp = 960,
                .up_sample_fs = 480,
                .bclk_div = 8,
            };

            esp_err_t result =
                i2s_channel_reconfig_pdm_tx_clock(tx_handle_, &tx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_tx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
#else
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "Requested I2S PDM direction is not supported by this chip\n");
            return false;
#endif
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
                i2s_channel_reconfig_std_clock(tx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }

            result = i2s_channel_reconfig_std_clock(rx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }

            break;
          }
          case I2sMode::kPdm: {
#if SOC_I2S_SUPPORTS_PDM_RX && SOC_I2S_SUPPORTS_PDM_TX
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

            esp_err_t result =
                i2s_channel_reconfig_pdm_rx_clock(rx_handle_, &rx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_rx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            result =
                i2s_channel_reconfig_pdm_tx_clock(tx_handle_, &tx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_tx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
#else
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "Requested I2S PDM direction is not supported by this chip\n");
            return false;
#endif
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
                i2s_channel_reconfig_std_clock(rx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_std_clock failed (error code: %#X)\n",
                  result);
              return false;
            }
          } break;
          case DataMode::kOutput: {
            esp_err_t result =
                i2s_channel_reconfig_std_clock(tx_handle_, &clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
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
#if SOC_I2S_SUPPORTS_PDM_RX
            i2s_pdm_rx_clk_config_t rx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .dn_sample_mode = I2S_PDM_DSR_8S,
                .bclk_div = 8,
            };

            esp_err_t result =
                i2s_channel_reconfig_pdm_rx_clock(rx_handle_, &rx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_rx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
#else
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "Requested I2S PDM direction is not supported by this chip\n");
            return false;
#endif
          }
          case DataMode::kOutput: {
#if SOC_I2S_SUPPORTS_PDM_TX
            i2s_pdm_tx_clk_config_t tx_clk_config = {
                .sample_rate_hz = sample_rate_hz,
                .clk_src = clock_source_,
                .mclk_multiple = mclk_multiple,
                .up_sample_fp = 960,
                .up_sample_fs = 480,
                .bclk_div = 8,
            };

            esp_err_t result =
                i2s_channel_reconfig_pdm_tx_clock(tx_handle_, &tx_clk_config);
            if (result != ESP_OK) {
              LogMessage(LogLevel::kError, __FILE__, __LINE__,
                  "i2s_channel_reconfig_pdm_tx_clock failed (error code: "
                  "%#X)\n",
                  result);
              return false;
            }

            break;
#else
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "Requested I2S PDM direction is not supported by this chip\n");
            return false;
#endif
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
          esp_err_t result = i2s_channel_enable(rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }

        case DataMode::kOutput: {
          esp_err_t result = i2s_channel_enable(tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kInputOutput: {
          esp_err_t result = i2s_channel_enable(tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }

          result = i2s_channel_enable(rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
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
          esp_err_t result = i2s_channel_enable(rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "i2s_channel_enable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kOutput: {
          esp_err_t result = i2s_channel_enable(tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
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
          esp_err_t result = i2s_channel_disable(rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kOutput: {
          esp_err_t result = i2s_channel_disable(tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kInputOutput: {
          esp_err_t result = i2s_channel_disable(tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }

          result = i2s_channel_disable(rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
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
          esp_err_t result = i2s_channel_disable(rx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
                "i2s_channel_disable failed (error code: %#X)\n", result);
            return false;
          }
          break;
        }
        case DataMode::kOutput: {
          esp_err_t result = i2s_channel_disable(tx_handle_);
          if (result != ESP_OK) {
            LogMessage(LogLevel::kError, __FILE__, __LINE__,
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

bool HardwareI2s::Deinit() {
  bool result = true;

  if (tx_handle_ != nullptr) {
    esp_err_t ret = i2s_channel_disable(tx_handle_);
    if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2s_channel_disable failed (error code: %#X)\n", ret);
      result = false;
    }

    ret = i2s_del_channel(tx_handle_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2s_del_channel failed (error code: %#X)\n", ret);
      result = false;
    } else {
      tx_handle_ = nullptr;
    }
  }

  if (rx_handle_ != nullptr) {
    esp_err_t ret = i2s_channel_disable(rx_handle_);
    if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2s_channel_disable failed (error code: %#X)\n", ret);
      result = false;
    }

    ret = i2s_del_channel(rx_handle_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "i2s_del_channel failed (error code: %#X)\n", ret);
      result = false;
    } else {
      rx_handle_ = nullptr;
    }
  }

  if ((tx_handle_ == nullptr) && (rx_handle_ == nullptr)) {
    if (data_in_ != kPinNotConnected) {
      result &= ResetGpio(data_in_);
    }
    if (data_out_ != kPinNotConnected) {
      result &= ResetGpio(data_out_);
    }
    if (ws_lrck_ != kPinNotConnected) {
      result &= ResetGpio(ws_lrck_);
    }
    if (bclk_ != kPinNotConnected) {
      result &= ResetGpio(bclk_);
    }
    if (mclk_ != kPinNotConnected) {
      result &= ResetGpio(mclk_);
    }
  }

  return result;
}
#endif

}  // namespace cpp_bus_driver

#endif
#endif
