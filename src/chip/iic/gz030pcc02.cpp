/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-07-24 13:46:14
 * @License: GPL 3.0
 */
#include "gz030pcc02.h"

namespace Cpp_Bus_Driver
{
    bool Gz030pcc02::begin(int32_t freq_hz)
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
        if (buffer != GZ030PCC02_DEVICE_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get gz030pcc02 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get gz030pcc02 id: %#X\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list)/sizeof(uint16_t)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_init_list fail\n");
            return false;
        }

        return true;
    }

    uint8_t Gz030pcc02::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint16_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

}
