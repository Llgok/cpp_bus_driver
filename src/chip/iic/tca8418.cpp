/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2025-07-30 14:33:02
 * @License: GPL 3.0
 */
#include "tca8418.h"

namespace Cpp_Bus_Driver
{
    bool Tca8418::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        uint8_t buffer = get_device_id();
        if (buffer != DEVICE_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get tca8418 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get tca8418 id: %#X\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        if (set_key_scan_window(0, 0, _width, _height) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_key_scan_window fail\n");
            return false;
        }

        return true;
    }

    uint8_t Tca8418::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Tca8418::set_key_scan_window(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
    {
        // 有效性检查
        if (w == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid width (error w = %d)", w);
            return false;
        }
        else if (h == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid height (error h = %d)", h);
            return false;
        }
        else if (x >= _width)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid x (error (x = %d) >= (_width = %d))", x, _width);
            return false;
        }
        else if (y >= _height)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid y (error (y = %d) >= (_height = %d))", y, _height);
            return false;
        }
        else if (w > (_width - x))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid width (error (w = %d) > ((_width - x) = %d))", w, _width - x);
            return false;
        }
        else if (h > (_height - y))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid height (error (h = %d) > ((_height - y) = %d))", h, _height - y);
            return false;
        }

        // 配置行选择寄存器
        uint8_t buffer_row_mask = 0;
        for (uint8_t i = y; i < h; i++)
        {
            buffer_row_mask |= (1 << i); // 设置对应的行位为1，键盘扫描
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_KP_GPIO1), buffer_row_mask) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 配置列选择寄存器
        uint8_t buffer_col_mask_low = 0;
        uint8_t buffer_col_mask_high = 0;
        for (uint8_t i = x; i < w; i++)
        {
            if (i < 8)
            {
                buffer_col_mask_low |= (1 << i); // 0~7列
            }
            else
            {
                buffer_col_mask_high |= (1 << (i - 8)); // 8，9列
            }
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_KP_GPIO2), buffer_col_mask_low) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_KP_GPIO3), buffer_col_mask_high) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }
}
