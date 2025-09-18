/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-09-18 11:16:24
 * @License: GPL 3.0
 */
#include "gz030pcc0x.h"

namespace Cpp_Bus_Driver
{
    bool Gz030pcc0x::begin(int32_t freq_hz)
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
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get gz030pcc02 id fail (error id: %#X)\n", buffer);
            // return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get gz030pcc02 id success (id: %#X)\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list) / sizeof(uint16_t)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        return true;
    }

    uint8_t Gz030pcc0x::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint16_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    float Gz030pcc0x::get_temperature_celsius(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint16_t>(Cmd::RO_TEMPERATURE_READING), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (0.51 * static_cast<float>(buffer)) - 63.0;
    }

    bool Gz030pcc0x::set_data_format(Data_Format format)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint16_t>(Cmd::RW_INTERNAL_TEST_MODE_INPUT_DATA_FORMAT), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B11111000) | static_cast<uint8_t>(format);

        if (_bus->write(static_cast<uint16_t>(Cmd::RW_INTERNAL_TEST_MODE_INPUT_DATA_FORMAT), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Gz030pcc0x::set_internal_test_mode(Internal_Test_Mode mode)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint16_t>(Cmd::RW_INTERNAL_TEST_MODE_INPUT_DATA_FORMAT), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B00011111) | static_cast<uint8_t>(mode);

        if (_bus->write(static_cast<uint16_t>(Cmd::RW_INTERNAL_TEST_MODE_INPUT_DATA_FORMAT), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Gz030pcc0x::set_show_direction(Show_Direction direction)
    {
        if (_bus->write(static_cast<uint16_t>(Cmd::RW_HORIZONTAL_VERTICAL_MIRROR), static_cast<uint8_t>(direction)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Gz030pcc0x::set_brightness(uint8_t value)
    {
        if (_bus->write(static_cast<uint16_t>(Cmd::RW_DISPLAY_BRIGHTNESS), value) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
