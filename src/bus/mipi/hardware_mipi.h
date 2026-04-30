/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-04-30 13:45:24
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
#if defined CPP_BUS_DRIVER_CHIP_ESP32P4
namespace cpp_bus_driver {
class HardwareMipi final : public BusMipiGuide {
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

  bool Init(float freq_mhz = CPP_BUS_DRIVER_DEFAULT_VALUE,
      float lane_bit_rate_mbps = CPP_BUS_DRIVER_DEFAULT_VALUE,
      InitSequenceFormat init_sequence_format =
          InitSequenceFormat::kWriteC8D8) override;

  bool StartTransmit() override;

  bool Read(int32_t cmd, void* data, size_t byte) override;
  bool Write(int32_t cmd, const void* data, size_t byte) override;
  bool Write(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end,
      const void* data) override;
  bool Deinit() override;

  bool set_device_handle(esp_lcd_panel_handle_t handle);

  esp_lcd_panel_handle_t device_handle();

 private:
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
