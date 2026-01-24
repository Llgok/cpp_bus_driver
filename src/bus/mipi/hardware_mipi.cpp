/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-01-24 17:21:06
 * @License: GPL 3.0
 */
#include "hardware_mipi.h"

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
namespace Cpp_Bus_Driver
{
    bool Hardware_Mipi::begin(int32_t freq_mhz, int32_t lane_bit_rate_mbps, Init_List_Format init_list_format)
    {
        if (freq_mhz == static_cast<int32_t>(CPP_BUS_DRIVER_DEFAULT_VALUE))
        {
            freq_mhz = CPP_BUS_DRIVER_DEFAULT_MIPI_FREQ_MHZ;
        }

        if (lane_bit_rate_mbps == static_cast<int32_t>(CPP_BUS_DRIVER_DEFAULT_VALUE))
        {
            lane_bit_rate_mbps = CPP_BUS_DRIVER_DEFAULT_MIPI_LANE_BIT_RATE_MBPS;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config freq_mhz: %d\n", freq_mhz);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config lane_bit_rate_mbps: %d\n", lane_bit_rate_mbps);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config init_list_format: %d\n", init_list_format);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _width: %d\n", _width);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _height: %d\n", _height);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _hsync: %d\n", _hsync);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _hbp: %d\n", _hbp);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _hfp: %d\n", _hfp);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _vsync: %d\n", _vsync);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _vbp: %d\n", _vbp);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _vfp: %d\n", _vfp);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _num_data_lane: %d\n", _num_data_lane);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _color_format: %d\n", _color_format);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _num_frame_buffer: %d\n", _num_frame_buffer);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _port: %d\n", _port);

        esp_lcd_dsi_bus_config_t bus_config =
            {
                .bus_id = _port,
                .num_data_lanes = _num_data_lane,
                .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
                .lane_bit_rate_mbps = static_cast<uint32_t>(lane_bit_rate_mbps),
            };

        esp_err_t assert = esp_lcd_new_dsi_bus(&bus_config, &_bus_handle);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "esp_lcd_new_dsi_bus fail (error code: %#X)\n", assert);
            return false;
        }

        esp_lcd_dbi_io_config_t io_config =
            {
                .virtual_channel = 0,
                .lcd_cmd_bits = [](Init_List_Format format) -> int
                {
                    switch (format)
                    {
                    case Init_List_Format::WRITE_C8_BYTE_DATA:
                        return 8;
                    case Init_List_Format::WRITE_C8_D8:
                        return 8;
                    default:
                        return 8;
                    }
                }(init_list_format),
                .lcd_param_bits = [](Init_List_Format format) -> int
                {
                    switch (format)
                    {
                    case Init_List_Format::WRITE_C8_BYTE_DATA:
                        return 8;
                    case Init_List_Format::WRITE_C8_D8:
                        return 8;
                    default:
                        return 8;
                    }
                }(init_list_format),
            };
        assert = esp_lcd_new_panel_io_dbi(_bus_handle, &io_config, &_io_handle);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "esp_lcd_new_panel_io_dbi fail (error code: %#X)\n", assert);
            return false;
        }

        esp_lcd_dpi_panel_config_t panel_config =
            {
                .virtual_channel = 0,
                .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
                .dpi_clock_freq_mhz = static_cast<uint32_t>(freq_mhz),
                .pixel_format = [](Color_Format format) -> lcd_color_rgb_pixel_format_t
                {
                    switch (format)
                    {
                    case Color_Format::RGB565:
                        return lcd_color_rgb_pixel_format_t::LCD_COLOR_PIXEL_FORMAT_RGB565;
                    case Color_Format::RGB666:
                        return lcd_color_rgb_pixel_format_t::LCD_COLOR_PIXEL_FORMAT_RGB666;
                    case Color_Format::RGB888:
                        return lcd_color_rgb_pixel_format_t::LCD_COLOR_PIXEL_FORMAT_RGB888;
                    default:
                        return lcd_color_rgb_pixel_format_t::LCD_COLOR_PIXEL_FORMAT_RGB565;
                    }
                }(_color_format),
                .in_color_format = [](Color_Format format) -> lcd_color_format_t
                {
                    switch (format)
                    {
                    case Color_Format::RGB565:
                        return lcd_color_format_t::LCD_COLOR_FMT_RGB565;
                    case Color_Format::RGB666:
                        return lcd_color_format_t::LCD_COLOR_FMT_RGB666;
                    case Color_Format::RGB888:
                        return lcd_color_format_t::LCD_COLOR_FMT_RGB888;
                    case Color_Format::YUV422:
                        return lcd_color_format_t::LCD_COLOR_FMT_YUV422;
                    default:
                        return lcd_color_format_t::LCD_COLOR_FMT_RGB565;
                    }
                }(_color_format),
                .out_color_format = [](Color_Format format) -> lcd_color_format_t
                {
                    switch (format)
                    {
                    case Color_Format::RGB565:
                        return lcd_color_format_t::LCD_COLOR_FMT_RGB565;
                    case Color_Format::RGB666:
                        return lcd_color_format_t::LCD_COLOR_FMT_RGB666;
                    case Color_Format::RGB888:
                        return lcd_color_format_t::LCD_COLOR_FMT_RGB888;
                    case Color_Format::YUV422:
                        return lcd_color_format_t::LCD_COLOR_FMT_YUV422;
                    default:
                        return lcd_color_format_t::LCD_COLOR_FMT_RGB565;
                    }
                }(_color_format),
                .num_fbs = _num_frame_buffer,
                .video_timing =
                    {
                        .h_size = _width,
                        .v_size = _height,
                        .hsync_pulse_width = _hsync,
                        .hsync_back_porch = _hbp,
                        .hsync_front_porch = _hfp,
                        .vsync_pulse_width = _vsync,
                        .vsync_back_porch = _vbp,
                        .vsync_front_porch = _vfp,
                    },
                .flags =
                    {
                        .use_dma2d = true,
                        .disable_lp = false,
                    }};

        assert = esp_lcd_new_panel_dpi(_bus_handle, &panel_config, &_device_handle);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "esp_lcd_new_panel_dpi fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Mipi::start_transmit(void)
    {
        esp_err_t assert = esp_lcd_panel_init(_device_handle);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "esp_lcd_panel_init fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Mipi::read(int32_t cmd, void *data, size_t byte)
    {
        esp_err_t assert = esp_lcd_panel_io_rx_param(_io_handle, cmd, data, byte);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "esp_lcd_panel_io_rx_param fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Mipi::write(int32_t cmd, const void *data, size_t byte)
    {
        esp_err_t assert = esp_lcd_panel_io_tx_param(_io_handle, cmd, data, byte);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "esp_lcd_panel_io_tx_param fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Mipi::set_device_handle(esp_lcd_panel_handle_t handle)
    {
        if (handle == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "set_device_handle fail (handle == nullptr)\n");
            return false;
        }

        _device_handle = handle;

        return true;
    }

    esp_lcd_panel_handle_t Hardware_Mipi::get_device_handle(void)
    {
        return _device_handle;
    }

    bool Hardware_Mipi::write(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, const uint8_t *data)
    {
        esp_err_t assert = esp_lcd_panel_draw_bitmap(_device_handle, static_cast<int>(x_start), x_end, y_start, y_end, data);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "esp_lcd_panel_draw_bitmap fail\n");
            return false;
        }

        return true;
    }

}

#endif
