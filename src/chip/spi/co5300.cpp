/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-07-03 09:10:33
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
        auto buffer_6 = std::make_unique<uint8_t[]>(466 * 466 * 16);
        memset(buffer_6.get(), 0xFF, 466 * 466 * 16);

        // while (1)
        // {
        //     if (set_render_window(100, 400, 100, 400) == false)
        //     {
        //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_render_window fail\n");
        //         return false;
        //     }

        //     _bus->write(0x32, 0x002C00, buffer_6.get(), 300 * 300 * 16, SPI_TRANS_MODE_QIO);
        //     delay_ms(1000);
        // }

        while (1)
        {
            memset(buffer_6.get(), 0xFF, 466 * 466 * 16);
            delay_ms(1000);
            send_color_stream(0, 466, 0, 466, buffer_6.get());
            delay_ms(1000);
            memset(buffer_6.get(), 0xAA, 466 * 466 * 16);
            send_color_stream(0, 466, 0, 466, buffer_6.get());
            delay_ms(1000);
            memset(buffer_6.get(), 0xBB, 466 * 466 * 16);
            send_color_stream(0, 466, 0, 466, buffer_6.get());
            delay_ms(1000);
            memset(buffer_6.get(), 0xCC, 466 * 466 * 16);
            send_color_stream(0, 466, 0, 466, buffer_6.get());
            delay_ms(1000);
        }

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

    bool Co5300::send_color_stream(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, const uint8_t *data)
    {
        uint16_t buffer_width = x_end - x_start;
        uint16_t buffer_height = y_end - y_start;
        // 需要绘制的总尺寸
        size_t buffer_size = buffer_width * buffer_height * static_cast<uint8_t>(_color_format);

        // 有效性检查
        if (buffer_width == 0 || buffer_height == 0 || data == nullptr)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid parameters: width=%d, height=%d, data=%p\n",
                       buffer_width, buffer_height, data);
            return false;
        }

        if (buffer_size > MAX_GRAM_SIZE_BYTES)
        {
            uint8_t *buffer_data_ptr = (uint8_t *)data;
            // 固定宽度，计算一次绘制的高度来确定刷新一次需要的数据量
            uint16_t buffer_height_refresh = MAX_GRAM_SIZE_BYTES / (buffer_width * static_cast<uint8_t>(_color_format));
            // 一次绘制高度所占的尺寸
            size_t buffer_height_refresh_size = buffer_width * buffer_height_refresh * static_cast<uint8_t>(_color_format);
            // 固定高度全部绘制完成后剩余需要绘制的高度
            uint16_t buffer_height_refresh_remain = buffer_height % buffer_height_refresh;

            uint16_t y_start_last = y_start;

            if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1),
                            static_cast<uint16_t>(Reg::WO_WRITE_COLOR_STREAM_1LANES_4LANES_ADDR_1)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            while (1)
            {
                if ((buffer_size / buffer_height_refresh_size) != 0)
                {
                    if (set_render_window(x_start, x_end, y_start_last, y_start_last + buffer_height_refresh) == false)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_render_window fail\n");
                        return false;
                    }

                    if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1),
                                    static_cast<uint16_t>(Reg::WO_WRITE_COLOR_STREAM_1LANES_4LANES_ADDR_1),
                                    buffer_data_ptr, buffer_height_refresh_size, SPI_TRANS_MODE_QIO) == false)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                        return false;
                    }

                    buffer_data_ptr += buffer_height_refresh_size;
                    buffer_size -= buffer_height_refresh_size;
                    y_start_last += buffer_height_refresh;
                }
                else
                {
                    // 只有剩余行数 >0 时才发送
                    if (buffer_height_refresh_remain > 0)
                    {
                        if (set_render_window(x_start, x_end, y_start_last, y_start_last + buffer_height_refresh_remain) == false)
                        {
                            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_render_window fail\n");
                            return false;
                        }

                        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1),
                                        static_cast<uint16_t>(Reg::WO_WRITE_COLOR_STREAM_1LANES_4LANES_ADDR_1),
                                        buffer_data_ptr, buffer_size, SPI_TRANS_MODE_QIO) == false)
                        {
                            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                            return false;
                        }
                    }
                    break;
                }
            }

            // 一个一个像素点输出
            //  uint8_t *buffer_data_ptr = (uint8_t *)data;
            //  for (uint16_t y = y_start; y < y_end; ++y)
            //  {
            //      for (uint16_t x = x_start; x < x_end; ++x)
            //      {
            //          if (set_render_window(x, x + 1, y, y + 1) == false)
            //          {
            //              assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_render_window fail\n");
            //              return false;
            //          }
            //          if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1),
            //                          static_cast<uint16_t>(Reg::WO_WRITE_COLOR_STREAM_1LANES_4LANES_ADDR_1),
            //                          buffer_data_ptr, static_cast<uint8_t>(_color_format), SPI_TRANS_MODE_QIO) == false)
            //          {
            //              assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            //              return false;
            //          }
            //          buffer_data_ptr += static_cast<uint8_t>(_color_format);
            //      }
            //  }
        }
        else
        {
            if (set_render_window(x_start, x_end, y_start, y_end) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_render_window fail\n");
                return false;
            }

            if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1),
                            static_cast<uint16_t>(Reg::WO_WRITE_COLOR_STREAM_1LANES_4LANES_ADDR_1),
                            data, buffer_size, SPI_TRANS_MODE_QIO) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        return true;
    }

}
