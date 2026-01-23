/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-01-23 18:05:16
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

    bool Hi8561::set_sleep(bool status)
    {
        // uint8_t buffer[] =
        //     {
        //         static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
        //         static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SLEEP_IN) >> 16),
        //         static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SLEEP_IN) >> 8),
        //         static_cast<uint8_t>(Reg::WO_SLEEP_IN),
        //     };

        // if (status == false)
        // {
        //     buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SLEEP_OUT) >> 16);
        //     buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SLEEP_OUT) >> 8);
        //     buffer[3] = static_cast<uint8_t>(Reg::WO_SLEEP_OUT);
        // }

        // if (_bus->write(buffer, 4) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }

        return true;
    }

    bool Hi8561::set_screen_off(bool status)
    {
        // uint8_t buffer[] =
        //     {
        //         static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
        //         static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_DISPLAY_OFF) >> 16),
        //         static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_DISPLAY_OFF) >> 8),
        //         static_cast<uint8_t>(Reg::WO_DISPLAY_OFF),
        //     };

        // if (status == false)
        // {
        //     buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_DISPLAY_ON) >> 16);
        //     buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_DISPLAY_ON) >> 8);
        //     buffer[3] = static_cast<uint8_t>(Reg::WO_DISPLAY_ON);
        // }

        // if (_bus->write(buffer, 4) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }

        return true;
    }

}
