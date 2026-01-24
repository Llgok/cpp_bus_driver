/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-01-24 17:21:56
 * @License: GPL 3.0
 */

#pragma once

#include "../bus_guide.h"

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
namespace Cpp_Bus_Driver
{
    class Hardware_Mipi : public Bus_Mipi_Guide
    {
    public:
        enum class Color_Format
        {
            RGB565,
            RGB666,
            RGB888,
            YUV422,
        };

    private:
        uint32_t _width, _height, _hsync, _hbp, _hfp, _vsync, _vbp, _vfp;
        uint8_t _num_data_lane;

        Color_Format _color_format;

        uint8_t _num_frame_buffer;

        int32_t _port;

        esp_lcd_dsi_bus_handle_t _bus_handle = nullptr;
        esp_lcd_panel_io_handle_t _io_handle = nullptr;
        esp_lcd_panel_handle_t _device_handle = nullptr;

    public:
        Hardware_Mipi(uint32_t width, uint32_t height, uint32_t hsync, uint32_t hbp, uint32_t hfp, uint32_t vsync, uint32_t vbp, uint32_t vfp,
                      uint8_t num_data_lane, Color_Format color_format, uint8_t num_frame_buffer = 0, int32_t port = 0)
            : _width(width), _height(height), _hsync(hsync), _hbp(hbp), _hfp(hfp), _vsync(vsync), _vbp(vbp), _vfp(vfp),
              _num_data_lane(num_data_lane), _color_format(color_format), _num_frame_buffer(num_frame_buffer), _port(port)
        {
        }

        bool begin(int32_t freq_mhz = CPP_BUS_DRIVER_DEFAULT_VALUE, int32_t lane_bit_rate_mbps = CPP_BUS_DRIVER_DEFAULT_VALUE,
                   Init_List_Format init_list_format = Init_List_Format::WRITE_C8_D8) override;

        bool start_transmit(void) override;

        bool read(int32_t cmd, void *data, size_t byte) override;
        bool write(int32_t cmd, const void *data, size_t byte) override;
        bool write(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, const uint8_t *data) override;

        bool set_device_handle(esp_lcd_panel_handle_t handle);

        esp_lcd_panel_handle_t get_device_handle(void);
    };

}
#endif