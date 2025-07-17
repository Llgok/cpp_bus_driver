/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-07-17 14:19:20
 * @License: GPL 3.0
 */
#include "sgm41562xx.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
    constexpr const uint8_t Sgm41562xx::_init_list[];
#endif

    bool Sgm41562xx::begin(int32_t freq_hz)
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
        if (buffer != SGM41562XX_DEVICE_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sgm41562xx id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sgm41562xx id: %#X\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_init_list fail\n");
            return false;
        }

        return true;
    }

    uint8_t Sgm41562xx::get_device_id(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    uint8_t Sgm41562xx::get_irq_flag(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_FAULT), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Sgm41562xx::parse_irq_status(uint8_t irq_flag, Irq_Status &status)
    {
        if (irq_flag == static_cast<uint8_t>(-1))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read error\n");
            return false;
        }

        status.vin_fault_flag = (irq_flag & 0B00100000) >> 5;
        status.thermal_shutdown_flag = (irq_flag & 0B00010000) >> 4;
        status.battery_over_voltage_fault_flag = (irq_flag & 0B00001000) >> 3;
        status.safety_timer_expiration_fault_flag = (irq_flag & 0B00000100) >> 2;
        status.ntc_exceeding_hot_flag = (irq_flag & 0B00000010) >> 1;
        status.ntc_exceeding_cold_flag = irq_flag & 0B00000001;

        return true;
    }

}
