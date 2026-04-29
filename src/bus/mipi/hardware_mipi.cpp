/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-04-29 09:30:51
 * @License: GPL 3.0
 */
#include "hardware_mipi.h"

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
#if defined CPP_BUS_DRIVER_CHIP_ESP32P4
namespace cpp_bus_driver {
bool HardwareMipi::Init(float freq_mhz, float lane_bit_rate_mbps,
    InitSequenceFormat init_sequence_format) {
  if ((bus_handle_ != nullptr) || (io_handle_ != nullptr) ||
      (device_handle_ != nullptr)) {
    if (!Deinit()) {
      LogMessage(
          LogLevel::kBus, __FILE__, __LINE__, "HardwareMipi deinit failed\n");
      return false;
    }
  }

  if (freq_mhz == static_cast<float>(CPP_BUS_DRIVER_DEFAULT_VALUE)) {
    freq_mhz = CPP_BUS_DRIVER_DEFAULT_MIPI_FREQ_MHZ;
  }

  if (lane_bit_rate_mbps == static_cast<float>(CPP_BUS_DRIVER_DEFAULT_VALUE)) {
    lane_bit_rate_mbps = CPP_BUS_DRIVER_DEFAULT_MIPI_LANE_BIT_RATE_MBPS;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config freq_mhz: %d\n", freq_mhz);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config lane_bit_rate_mbps: %f\n", lane_bit_rate_mbps);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config init_sequence_format: %d\n", init_sequence_format);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config width_: %d\n", width_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config height_: %d\n", height_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config hsync_: %d\n", hsync_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config hbp_: %d\n", hbp_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config hfp_: %d\n", hfp_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config vsync_: %d\n", vsync_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config vbp_: %d\n", vbp_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config vfp_: %d\n", vfp_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config num_data_lane_: %d\n", num_data_lane_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config color_format_: %d\n", color_format_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config num_frame_buffer_: %d\n", num_frame_buffer_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareMipi config port_: %d\n", port_);

  esp_lcd_dsi_bus_config_t bus_config = {
      .bus_id = port_,
      .num_data_lanes = num_data_lane_,
      .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
      .lane_bit_rate_mbps = static_cast<float>(lane_bit_rate_mbps),
  };

  esp_err_t result = esp_lcd_new_dsi_bus(&bus_config, &bus_handle_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "esp_lcd_new_dsi_bus failed (error code: %#X)\n", result);
    return false;
  }

  esp_lcd_dbi_io_config_t io_config = {
      .virtual_channel = 0,
      .lcd_cmd_bits = [this](InitSequenceFormat format) -> int {
        switch (format) {
          case InitSequenceFormat::kWriteC8ByteData:
            return 8;
          case InitSequenceFormat::kWriteC8D8:
            return 8;
          default:
            return 8;
        }
      }(init_sequence_format),
      .lcd_param_bits = [this](InitSequenceFormat format) -> int {
        switch (format) {
          case InitSequenceFormat::kWriteC8ByteData:
            return 8;
          case InitSequenceFormat::kWriteC8D8:
            return 8;
          default:
            return 8;
        }
      }(init_sequence_format),
  };
  result = esp_lcd_new_panel_io_dbi(bus_handle_, &io_config, &io_handle_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "esp_lcd_new_panel_io_dbi failed (error code: %#X)\n", result);
    return false;
  }

  esp_lcd_dpi_panel_config_t panel_config = {.virtual_channel = 0,
      .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
      .dpi_clock_freq_mhz = static_cast<float>(freq_mhz),
      .pixel_format = [](ColorFormat format) -> lcd_color_rgb_pixel_format_t {
        switch (format) {
          case ColorFormat::kRgb565:
            return lcd_color_rgb_pixel_format_t::LCD_COLOR_PIXEL_FORMAT_RGB565;
          case ColorFormat::kRgb666:
            return lcd_color_rgb_pixel_format_t::LCD_COLOR_PIXEL_FORMAT_RGB666;
          case ColorFormat::kRgb888:
            return lcd_color_rgb_pixel_format_t::LCD_COLOR_PIXEL_FORMAT_RGB888;
          default:
            return lcd_color_rgb_pixel_format_t::LCD_COLOR_PIXEL_FORMAT_RGB565;
        }
      }(color_format_),
      .in_color_format = [](ColorFormat format) -> lcd_color_format_t {
        switch (format) {
          case ColorFormat::kRgb565:
            return lcd_color_format_t::LCD_COLOR_FMT_RGB565;
          case ColorFormat::kRgb666:
            return lcd_color_format_t::LCD_COLOR_FMT_RGB666;
          case ColorFormat::kRgb888:
            return lcd_color_format_t::LCD_COLOR_FMT_RGB888;
          case ColorFormat::kYuv422:
            return lcd_color_format_t::LCD_COLOR_FMT_YUV422;
          default:
            return lcd_color_format_t::LCD_COLOR_FMT_RGB565;
        }
      }(color_format_),
      .out_color_format = [](ColorFormat format) -> lcd_color_format_t {
        switch (format) {
          case ColorFormat::kRgb565:
            return lcd_color_format_t::LCD_COLOR_FMT_RGB565;
          case ColorFormat::kRgb666:
            return lcd_color_format_t::LCD_COLOR_FMT_RGB666;
          case ColorFormat::kRgb888:
            return lcd_color_format_t::LCD_COLOR_FMT_RGB888;
          case ColorFormat::kYuv422:
            return lcd_color_format_t::LCD_COLOR_FMT_YUV422;
          default:
            return lcd_color_format_t::LCD_COLOR_FMT_RGB565;
        }
      }(color_format_),
      .num_fbs = num_frame_buffer_,
      .video_timing =
          {
              .h_size = width_,
              .v_size = height_,
              .hsync_pulse_width = hsync_,
              .hsync_back_porch = hbp_,
              .hsync_front_porch = hfp_,
              .vsync_pulse_width = vsync_,
              .vsync_back_porch = vbp_,
              .vsync_front_porch = vfp_,
          },
      .flags =
          {
              .use_dma2d = true,
              .disable_lp = false,
          }};

  result = esp_lcd_new_panel_dpi(bus_handle_, &panel_config, &device_handle_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "esp_lcd_new_panel_dpi failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareMipi::StartTransmit() {
  esp_err_t result = esp_lcd_panel_init(device_handle_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "esp_lcd_panel_init failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareMipi::Deinit() {
  bool result = true;

  if (device_handle_ != nullptr) {
    esp_err_t ret = esp_lcd_panel_del(device_handle_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "esp_lcd_panel_del failed (error code: %#X)\n", ret);
      result = false;
    } else {
      device_handle_ = nullptr;
    }
  }

  if (io_handle_ != nullptr) {
    esp_err_t ret = esp_lcd_panel_io_del(io_handle_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "esp_lcd_panel_io_del failed (error code: %#X)\n", ret);
      result = false;
    } else {
      io_handle_ = nullptr;
    }
  }

  if (bus_handle_ != nullptr) {
    esp_err_t ret = esp_lcd_del_dsi_bus(bus_handle_);
    if (ret != ESP_OK) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__,
          "esp_lcd_del_dsi_bus failed (error code: %#X)\n", ret);
      result = false;
    } else {
      bus_handle_ = nullptr;
    }
  }

  return result;
}

bool HardwareMipi::Read(int32_t cmd, void* data, size_t byte) {
  esp_err_t result = esp_lcd_panel_io_rx_param(io_handle_, cmd, data, byte);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "esp_lcd_panel_io_rx_param failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareMipi::Write(int32_t cmd, const void* data, size_t byte) {
  esp_err_t result = esp_lcd_panel_io_tx_param(io_handle_, cmd, data, byte);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "esp_lcd_panel_io_tx_param failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareMipi::set_device_handle(esp_lcd_panel_handle_t handle) {
  if (handle == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  device_handle_ = handle;

  return true;
}

esp_lcd_panel_handle_t HardwareMipi::device_handle() { return device_handle_; }

bool HardwareMipi::Write(uint16_t x_start, uint16_t x_end, uint16_t y_start,
    uint16_t y_end, const void* data) {
  esp_err_t result = esp_lcd_panel_draw_bitmap(
      device_handle_, static_cast<int>(x_start), x_end, y_start, y_end, data);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "esp_lcd_panel_draw_bitmap failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver

#endif
#endif
