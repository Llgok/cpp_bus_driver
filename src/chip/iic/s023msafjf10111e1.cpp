/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-03-19 09:48:40
 * @License: GPL 3.0
 */
#include "s023msafjf10111e1.h"

namespace Cpp_Bus_Driver
{
    bool S023msafjf10111e1::begin(int32_t freq_hz)
    {
        if (_rst != CPP_BUS_DRIVER_DEFAULT_VALUE)
        {
            pin_mode(_rst, Pin_Mode::OUTPUT, Pin_Status::PULLUP);

            pin_write(_rst, 1);
            delay_ms(10);
            pin_write(_rst, 0);
            delay_ms(10);
            pin_write(_rst, 1);
            delay_ms(10);
        }

        if (Chip_Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool S023msafjf10111e1::set_data_format(Data_Format format)
    {
        switch (format)
        {
        case Data_Format::RGB888:
            if (_bus->write(static_cast<uint16_t>(Cmd::RW_INTERNAL_TEST_MODE_REGISTER_CONTROL_1), 0x14) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            if (_bus->write(static_cast<uint16_t>(Cmd::RW_INTERNAL_TEST_MODE_REGISTER_CONTROL_2), 0x40) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Data_Format::INTERNAL_TEST_MODE:
            if (_bus->write(static_cast<uint16_t>(Cmd::RW_INTERNAL_TEST_MODE_REGISTER_CONTROL_1), 0x15) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            if (_bus->write(static_cast<uint16_t>(Cmd::RW_INTERNAL_TEST_MODE_REGISTER_CONTROL_2), 0x80) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;

        default:
            break;
        }

        return true;
    }

    bool S023msafjf10111e1::set_internal_test_mode(Internal_Test_Mode mode)
    {
        if (_bus->write(static_cast<uint16_t>(Cmd::RW_INTERNAL_TEST_MODE), static_cast<uint8_t>(mode)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool S023msafjf10111e1::set_show_direction(Show_Direction direction)
    {
        // if (_bus->write(static_cast<uint16_t>(Cmd::RW_HORIZONTAL_VERTICAL_MIRROR), static_cast<uint8_t>(direction)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }

        return true;
    }

    bool S023msafjf10111e1::set_brightness(uint16_t value)
    {
        if (value > 511)
        {
            value = 511;
        }

        if (_bus->write(static_cast<uint16_t>(Cmd::RW_DISPLAY_BRIGHTNESS_REGISTER_CONTROL_1), 0x1C) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint16_t>(Cmd::RW_DISPLAY_BRIGHTNESS_REGISTER_CONTROL_2), 0x03) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint16_t>(Cmd::RW_DISPLAY_BRIGHTNESS_1), static_cast<uint8_t>(value)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint16_t>(Cmd::RW_DISPLAY_BRIGHTNESS_2), static_cast<uint8_t>(value >> 8)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint16_t>(Cmd::RW_DISPLAY_BRIGHTNESS_REGISTER_CONTROL_1), 0x15) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint16_t>(Cmd::RW_DISPLAY_BRIGHTNESS_REGISTER_CONTROL_2), 0x01) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
