/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-01-24 17:39:05
 * @License: GPL 3.0
 */
#include "rm69a10.h"

namespace Cpp_Bus_Driver
{
    bool Rm69a10::begin(int32_t freq_mhz, int32_t lane_bit_rate_mbps)
    {
        if (_rst != CPP_BUS_DRIVER_DEFAULT_VALUE)
        {
            pin_mode(_rst, Pin_Mode::OUTPUT, Pin_Status::PULLUP);

            pin_write(_rst, 1);
            delay_ms(5);
            pin_write(_rst, 0);
            delay_ms(10);
            pin_write(_rst, 1);
            delay_ms(120);
        }

        if (Chip_Mipi_Guide::begin(freq_mhz, lane_bit_rate_mbps) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        auto buffer = get_device_id();
        if (buffer != DEVICE_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get rm69a10 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get rm69a10 id success (id: %#X)\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        if (_bus->start_transmit() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "start_transmit fail\n");
            return false;
        }

        return true;
    }

    uint8_t Rm69a10::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer, 1) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Rm69a10::set_sleep(bool enable)
    {
        if (_bus->write(enable ? static_cast<uint8_t>(Cmd::WO_SLPIN) : static_cast<uint8_t>(Cmd::WO_SLPOUT)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        delay_ms(120);

        return true;
    }

    bool Rm69a10::set_screen_off(bool enable)
    {
        if (_bus->write(enable ? static_cast<uint8_t>(Cmd::WO_DISPOFF) : static_cast<uint8_t>(Cmd::WO_DISPON)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Rm69a10::set_inversion(bool enable)
    {
        if (_bus->write(enable ? static_cast<uint8_t>(Cmd::WO_INVON) : static_cast<uint8_t>(Cmd::WO_INVOFF)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_inversion write fail\n");
            return false;
        }

        return true;
    }

    bool Rm69a10::set_brightness(uint8_t brightness)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRDISBV), brightness) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_brightness write fail\n");
            return false;
        }

        return true;
    }

    bool Rm69a10::send_color_stream_coordinate(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, const uint8_t *data)
    {
        if (_bus->write(x_start, x_end, y_start, y_end, data) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }
}
