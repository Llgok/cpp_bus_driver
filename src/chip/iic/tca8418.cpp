/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2025-07-30 11:09:00
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
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_init_list fail\n");
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
}
