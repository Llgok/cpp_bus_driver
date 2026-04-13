/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2026-04-13 17:14:40
 * @License: GPL 3.0
 */
#include "chip_guide.h"

namespace Cpp_Bus_Driver
{
    bool Chip_Iic_Guide::begin(int32_t freq_hz)
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

    bool Chip_Iic_Guide::end(void)
    {
        if (_bus->end() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "end fail\n");
            return false;
        }

        return true;
    }

    bool Chip_Iic_Guide::init_list(const uint8_t *list, size_t length)
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

    bool Chip_Iic_Guide::init_list(const uint16_t *list, size_t length)
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

    bool Chip_Spi_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz, _cs) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Chip_Spi_Guide::init_list(const uint8_t *list, size_t length)
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

    bool Chip_Qspi_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz, _cs) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Chip_Qspi_Guide::init_list(const uint32_t *list, size_t length)
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

    bool Chip_Uart_Guide::begin(int32_t baud_rate)
    {
        if (_bus->begin(baud_rate) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Chip_Iis_Guide::begin(uint16_t mclk_multiple, uint32_t sample_rate_hz, uint8_t data_bit_width)
    {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
        if (_bus->begin([this](uint16_t mm) -> i2s_mclk_multiple_t
                        {
                            if (mm <= 128)
                            {
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128;
                            }
                            else if (mm <= 192)
                            {
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_192;
                            }
                            else if (mm <= 256)
                            {
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
                            }
                            else if (mm <= 384)
                            {
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_384;
                            }
                            else if (mm <= 512)
                            {
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_512;
                            }
                            else if (mm <= 576)
                            {
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_576;
                            }
                            else if (mm <= 768)
                            {
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_768;
                            }
                            else if (mm <= 1024)
                            {
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1024;
                            }
                            else if (mm <= 1152)
                            {
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1152;
                            }
                            else
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "setting out of bounds\n");
                                return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
                            } }(mclk_multiple), sample_rate_hz, [this](uint8_t dbw) -> i2s_data_bit_width_t
                        {
                            if (dbw <= 16)
                            {
                                return i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
                            }
                            else if (dbw <= 24)
                            {
                                return i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_24BIT;
                            }
                            else if (dbw <= 32)
                            {
                                return i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_32BIT;
                            }
                            else
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "setting out of bounds\n");
                                return i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
                            } }(data_bit_width)) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
#else
        assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
        return false;
#endif
    }

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
    bool Chip_Iis_Guide::set_clock_reconfig(uint16_t mclk_multiple, uint32_t sample_rate_hz, Bus_Iis_Guide::Data_Mode data_mode)
    {
        if (_bus->set_clock_reconfig([this](uint16_t mm) -> i2s_mclk_multiple_t
                                     {
                                        if (mm <= 128)
                                        {
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128;
                                        }
                                        else if (mm <= 192)
                                        {
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_192;
                                        }
                                        else if (mm <= 256)
                                        {
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
                                        }
                                        else if (mm <= 384)
                                        {
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_384;
                                        }
                                        else if (mm <= 512)
                                        {
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_512;
                                        }
                                        else if (mm <= 576)
                                        {
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_576;
                                        }
                                        else if (mm <= 768)
                                        {
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_768;
                                        }
                                        else if (mm <= 1024)
                                        {
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1024;
                                        }
                                        else if (mm <= 1152)
                                        {
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1152;
                                        }
                                        else
                                        {
                                            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "setting out of bounds\n");
                                            return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
                                        } }(mclk_multiple), sample_rate_hz, data_mode) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }
#endif

    bool Chip_Sdio_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Chip_Mipi_Guide::begin(float freq_mhz, float lane_bit_rate_mbps)
    {
        if (_bus->begin(freq_mhz, lane_bit_rate_mbps, _init_list_format) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Chip_Mipi_Guide::init_list(const uint8_t *list, size_t length)
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
                if (_bus->write(static_cast<int32_t>(list[index]), nullptr, 0) == false)
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