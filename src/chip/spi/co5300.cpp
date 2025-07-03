/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-07-03 18:17:04
 * @License: GPL 3.0
 */
#include "co5300.h"

namespace Cpp_Bus_Driver
{
    bool Co5300::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            pin_mode(_rst, Pin_Mode::OUTPUT, Pin_Status::PULLUP);

            pin_write(_rst, 1);
            delay_ms(10);
            pin_write(_rst, 0);
            delay_ms(10);
            pin_write(_rst, 1);
            delay_ms(10);
        }

        if (Qspi_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        if (init_list(_init_list, sizeof(_init_list) / sizeof(uint16_t)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_init_list fail\n");
            return false;
        }

        // 将buffer_6的内容全部改为0xFF，表示白色（通常RGB565或RGB888白色为全1）
        // void * buffer_6 = heap_caps_malloc(466 * 466 * 16, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        // auto buffer_6 = std::make_unique<uint8_t[]>(466 * 466 * 16);
        // memset(buffer_6.get(), 0xFF, 466 * 466 * 16);
        // send_color_stream(0, 471, 0, 467, buffer_6.get());

        // while (1)
        // {
        //     memset(buffer_6.get(), 0xFF, 466 * 466 * 16);
        //     delay_ms(1000);
        //     send_color_stream(0, 471, 0, 467, buffer_6.get());
        //     delay_ms(1000);
        //     memset(buffer_6.get(), 0xAA, 466 * 466 * 16);
        //     send_color_stream(0, 471, 0, 467, buffer_6.get());
        //     delay_ms(1000);
        //     memset(buffer_6.get(), 0xBB, 466 * 466 * 16);
        //     send_color_stream(0, 471, 0, 467, buffer_6.get());
        //     delay_ms(1000);
        //     memset(buffer_6.get(), 0xCC, 466 * 466 * 16);
        //     send_color_stream(0, 471, 0, 467, buffer_6.get());
        //     delay_ms(1000);
        // }

        return true;
    }

    bool Co5300::set_render_window(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end)
    {
        x_start += _x_offset;
        x_end += _x_offset;
        y_start += _y_offset;
        y_end += _y_offset;

        uint8_t buffer[] =
            {
                static_cast<uint8_t>(x_start >> 8),
                static_cast<uint8_t>(x_start),
                static_cast<uint8_t>(x_end >> 8),
                static_cast<uint8_t>(x_end),
            };
        uint8_t buffer_2[] =
            {
                static_cast<uint8_t>(y_start >> 8),
                static_cast<uint8_t>(y_start),
                static_cast<uint8_t>(y_end >> 8),
                static_cast<uint8_t>(y_end),
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::WO_COLUMN_ADDRESS_SET), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::WO_PAGE_ADDRESS_SET), buffer_2, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::WO_MEMORY_WRITE_START)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::send_color_stream(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data)
    {
        // 有效性检查
        if (w == 0 || h == 0 || data == nullptr)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid parameters: w=%d, h=%d, data=%p\n", w, h, data);
            return false;
        }

        if (set_render_window(x, x + w, y, y + h) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_render_window fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1),
                        static_cast<uint16_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE),
                        data, w * h * (static_cast<uint8_t>(_color_format) / 8), SPI_TRANS_MODE_QIO) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::draw_point(uint16_t x, uint16_t y, const uint8_t *color_data)
    {
        if (set_render_window(x, x + 1, y, y + 1) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_render_window fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1),
                        static_cast<uint16_t>(Reg::WO_MEMORY_START_WRITE),
                        color_data, 2 * 2 * static_cast<uint8_t>(_color_format) / 8, SPI_TRANS_MODE_QIO) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
