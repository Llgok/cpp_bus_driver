/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2026-01-23 17:26:24
 * @License: GPL 3.0
 */
#include "chip_guide.h"

namespace Cpp_Bus_Driver
{
    bool Iic_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz, _address) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        if (_bus->probe(_address) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "probe fail (not found address: %#X)\n", _address);
            return false;
        }

        return true;
    }

    bool Iic_Guide::end(void)
    {
        if (_bus->end() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "end fail\n");
            return false;
        }

        return true;
    }

    bool Iic_Guide::init_list(const uint8_t *list, size_t length)
    {
        size_t index = 0;
        while (index < length)
        {
            switch (list[index])
            {
            case static_cast<uint8_t>(Init_List_Format::DELAY_MS):
                index++;
                delay_ms(list[index]);
                index++;
                break;
            case static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8):
                index++;
                if (_bus->write(&list[index], 2) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list WRITE_C8_D8 fail\n");
                    return false;
                }
                index += 2;
                break;

            default:
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail (error index = %d)\n", index);
                return false;
                break;
            }
        }

        return true;
    }

    bool Iic_Guide::init_list(const uint16_t *list, size_t length)
    {
        size_t index = 0;
        while (index < length)
        {
            switch (list[index])
            {
            case static_cast<uint8_t>(Init_List_Format::DELAY_MS):
                index++;
                delay_ms(list[index]);
                index++;
                break;
            case static_cast<uint8_t>(Init_List_Format::WRITE_C16_D8):
            {
                index++;
                const uint8_t buffer[] =
                    {
                        static_cast<uint8_t>(list[index] >> 8),
                        static_cast<uint8_t>(list[index]),
                        static_cast<uint8_t>(list[index + 1]),
                    };

                if (_bus->write(buffer, 3) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list WRITE_C16_D8 fail\n");
                    return false;
                }
                index += 2;
                break;
            }
            default:
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail (error index = %d)\n", index);
                return false;
                break;
            }
        }

        return true;
    }

    bool Spi_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz, _cs) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Spi_Guide::init_list(const uint8_t *list, size_t length)
    {
        size_t index = 0;
        while (index < length)
        {
            switch (list[index])
            {
            case static_cast<uint8_t>(Init_List_Format::DELAY_MS):
                index++;
                delay_ms(list[index]);
                index++;
                break;
            case static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8):
                index++;
                if (_bus->write(&list[index], 2) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "spi_init_list WRITE_C8_D8 fail (index = %d)\n", index);
                    return false;
                }
                index += 2;
                break;

            default:
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail (error index = %d)\n", index);
                return false;
                break;
            }
        }

        return true;
    }

    bool Qspi_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz, _cs) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Qspi_Guide::init_list(const uint32_t *list, size_t length)
    {
        size_t index = 0;
        while (index < length)
        {
            switch (list[index])
            {
            case static_cast<uint8_t>(Init_List_Format::DELAY_MS):
                index++;
                delay_ms(list[index]);
                index++;
                break;
            case static_cast<uint8_t>(Init_List_Format::WRITE_C8_R24):
            {
                index++;
                uint8_t buffer[] =
                    {
                        static_cast<uint8_t>(list[index]),
                        static_cast<uint8_t>(list[index + 1] >> 16),
                        static_cast<uint8_t>(list[index + 1] >> 8),
                        static_cast<uint8_t>(list[index + 1]),
                    };
                if (_bus->write(buffer, 4) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "qspi_init_list WRITE_C8_R24 fail (index = %d)\n", index);
                    return false;
                }
                index += 2;

                break;
            }

            case static_cast<uint8_t>(Init_List_Format::WRITE_C8_R24_D8):
            {
                index++;
                uint8_t buffer[] =
                    {
                        static_cast<uint8_t>(list[index]),
                        static_cast<uint8_t>(list[index + 1] >> 16),
                        static_cast<uint8_t>(list[index + 1] >> 8),
                        static_cast<uint8_t>(list[index + 1]),
                        static_cast<uint8_t>(list[index + 2]),
                    };

                if (_bus->write(buffer, 5) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "qspi_init_list WRITE_C8_R24_D8 fail (index = %d)\n", index);
                    return false;
                }
                index += 3;

                break;
            }

            default:
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail (error index = %d)\n", index);
                return false;
                break;
            }
        }

        return true;
    }

    bool Uart_Guide::begin(int32_t baud_rate)
    {
        if (_bus->begin(baud_rate) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
    bool Iis_Guide::begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width)
    {
        if (_bus->begin(mclk_multiple, sample_rate_hz, data_bit_width) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
    bool Iis_Guide::begin(nrf_i2s_ratio_t mclk_multiple, uint32_t sample_rate_hz, nrf_i2s_swidth_t data_bit_width, nrf_i2s_channels_t channel)
    {
        if (_bus->begin(mclk_multiple, sample_rate_hz, data_bit_width, channel) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }
#endif

    bool Sdio_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Mipi_Guide::begin(int32_t freq_mhz, int32_t lane_bit_rate_mbps)
    {
        if (_bus->begin(freq_mhz, lane_bit_rate_mbps, _init_list_format) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Mipi_Guide::init_list(const uint8_t *list, size_t length)
    {
        size_t index = 0;
        while (index < length)
        {
            switch (list[index])
            {
            case static_cast<uint8_t>(Init_List_Format::DELAY_MS):
                index++;
                delay_ms(list[index]);
                index++;
                break;
            case static_cast<uint8_t>(Init_List_Format::WRITE_C8):
                index++;
                if (_bus->write(static_cast<int32_t>(list[index]), static_cast<uint8_t[]>(0x00), 0) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "mipi_init_list WRITE_C8 fail (index = %d)\n", index);
                    return false;
                }
                index++;
                break;
            case static_cast<uint8_t>(Init_List_Format::WRITE_C8_BYTE_DATA):
                index++;
                if (_bus->write(static_cast<int32_t>(list[index]), &list[index + 2], list[index + 1]) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "mipi_init_list WRITE_C8_BYTE_DATA fail (index = %d)\n", index);
                    return false;
                }
                index += list[index + 1] + 2;
                break;
            case static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8):
                index++;
                if (_bus->write(static_cast<int32_t>(list[index]), &list[index + 1], 1) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "mipi_init_list WRITE_C8_D8 fail (index = %d)\n", index);
                    return false;
                }
                index += 2;
                break;

            default:
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail (error index = %d)\n", index);
                return false;
                break;
            }
        }

        return true;
    }
}