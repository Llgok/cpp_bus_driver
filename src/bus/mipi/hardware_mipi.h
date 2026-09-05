/*
 * @Description: ESP-IDF MIPI-DSI 显示总线驱动接口
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-09-05 08:57:19
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "bus/bus_base.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

namespace cpp_bus_driver {
class HardwareMipi final : public MipiBusBase {
 public:
  enum class ColorFormat {
    kRgb565,
    kRgb666,
    kRgb888,
    kYuv422,
  };

  explicit HardwareMipi(uint32_t width, uint32_t height, uint32_t hsync,
      uint32_t hbp, uint32_t hfp, uint32_t vsync, uint32_t vbp, uint32_t vfp,
      uint8_t num_data_lane, ColorFormat color_format,
      uint8_t num_frame_buffer = 0, int32_t port = 0)
      : width_(width),
        height_(height),
        hsync_(hsync),
        hbp_(hbp),
        hfp_(hfp),
        vsync_(vsync),
        vbp_(vbp),
        vfp_(vfp),
        num_data_lane_(num_data_lane),
        color_format_(color_format),
        num_frame_buffer_(num_frame_buffer),
        port_(port) {}

  bool Init(float freq_mhz = kDefaultFrequencyMhz,
      float lane_bit_rate_mbps = kDefaultLaneBitRateMbps,
      InitSequenceFormat init_sequence_format =
          InitSequenceFormat::kWriteC8D8) override;

  bool StartTransmit() override;

  bool Read(int32_t cmd, void* data, size_t byte) override;
  bool Write(int32_t cmd, const void* data, size_t byte) override;
  bool Write(int x_start, int y_start, int x_end, int y_end,
      const void* data) override;
  bool Deinit() override;
  bool set_device_handle(esp_lcd_panel_handle_t handle);
  esp_lcd_panel_handle_t device_handle();

 private:
  // 默认像素时钟和 DSI 数据通道速率，单位分别为 MHz 和 Mbps。
  static constexpr float kDefaultFrequencyMhz = 60.0F;
  static constexpr float kDefaultLaneBitRateMbps = 1000.0F;

  uint32_t width_, height_, hsync_, hbp_, hfp_, vsync_, vbp_, vfp_;
  uint8_t num_data_lane_;
  ColorFormat color_format_;
  uint8_t num_frame_buffer_;
  int32_t port_;
  esp_lcd_dsi_bus_handle_t bus_handle_ = nullptr;
  esp_lcd_panel_io_handle_t io_handle_ = nullptr;
  esp_lcd_panel_handle_t device_handle_ = nullptr;
};

}  // namespace cpp_bus_driver
#endif
#endif
