/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2025-08-22 09:51:26
 * @License: GPL 3.0
 */
#include "software_iic.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
    bool Software_Iic::begin(uint32_t freq_hz, uint16_t address)
    {
        if (freq_hz == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            freq_hz = DEFAULT_CPP_BUS_DRIVER_IIC_FREQ_HZ;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config address: %#X\n", address);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config _init_flag: %d\n", _init_flag);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config _sda: %d\n", _sda);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config _scl: %d\n", _scl);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config freq_hz: %d hz\n", freq_hz);

        _freq_hz = freq_hz;
        _address = address;

        return true;
    }

    bool Software_Iic::read(uint8_t *data, size_t length)
    {

        return true;
    }
    bool Software_Iic::write(const uint8_t *data, size_t length)
    {

        return true;
    }
    bool Software_Iic::write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length)
    {

        return true;
    }

    bool Software_Iic::probe(const uint16_t address)
    {

        return true;
    }

#endif
}