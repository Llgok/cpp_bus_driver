/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-01-24 17:33:42
 * @License: GPL 3.0
 */
#include "hi8561.h"

namespace Cpp_Bus_Driver
{
    bool Hi8561::begin(int32_t freq_mhz, int32_t lane_bit_rate_mbps)
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

        if (Mipi_Guide::begin(freq_mhz, lane_bit_rate_mbps) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        auto buffer = get_device_id();
        if (buffer != DEVICE_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get hi8561 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get hi8561 id success (id: %#X)\n", buffer);
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

    uint16_t Hi8561::get_device_id(void)
    {
        uint8_t buffer[2] = {0};

        for (uint8_t i = 0; i < 2; i++)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID_START) + i, &buffer[i], 1) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return -1;
            }
        }

        return (static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]);
    }

    bool Hi8561::set_sleep(bool enable)
    {
        if (_bus->write(enable ? static_cast<uint8_t>(Cmd::WO_SLPIN) : static_cast<uint8_t>(Cmd::WO_SLPOUT)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        delay_ms(120);

        return true;
    }

    bool Hi8561::set_screen_off(bool enable)
    {
        if (_bus->write(enable ? static_cast<uint8_t>(Cmd::WO_DISPOFF) : static_cast<uint8_t>(Cmd::WO_DISPON)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Hi8561::set_mirror(Mirror_Mode mode)
    {
        _madctl_data &= 0B11111100;
        switch (mode)
        {
        case Mirror_Mode::OFF:
            break;

        case Mirror_Mode::HORIZONTAL:
            _madctl_data |= 0B00000010;
            break;

        case Mirror_Mode::VERTICAL:
            _madctl_data |= 0B00000001;
            break;

        case Mirror_Mode::HORIZONTAL_VERTICAL:
            _madctl_data |= 0B00000011;
            break;

        default:
            break;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_MADCTL), _madctl_data) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Hi8561::set_inversion(bool enable)
    {
        if (_bus->write(enable ? static_cast<uint8_t>(Cmd::WO_INVON) : static_cast<uint8_t>(Cmd::WO_INVOFF)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_inversion write fail\n");
            return false;
        }

        return true;
    }

    bool Hi8561::set_brightness(uint8_t brightness)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRDISBV), brightness) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_brightness write fail\n");
            return false;
        }

        return true;
    }

    bool Hi8561::set_color_order(Color_Order order)
    {
        _madctl_data = (_madctl_data & 0xB11110111) | (static_cast<uint8_t>(order) << 3);

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_MADCTL), _madctl_data) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Hi8561::set_cabc_mode(Cabc_Mode mode)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRCABC), static_cast<uint8_t>(mode)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Hi8561::send_color_stream_coordinate(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, const uint8_t *data)
    {
        if (_bus->write(x_start, x_end, y_start, y_end, data) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
