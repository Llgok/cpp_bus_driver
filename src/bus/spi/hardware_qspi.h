/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2025-06-27 09:23:06
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Hardware_Qspi : public Bus_Qspi_Guide
    {
    private:
        int32_t _data0, _data1, _data2, _data3, _sclk, _cs, _freq_hz;
        spi_host_device_t _port;
        uint8_t _mode;
        uint32_t _flags;

        spi_device_handle_t _spi_device;

    public:
        Hardware_Qspi(int32_t data0, int32_t data1, int32_t data2, int32_t data3, int32_t sclk,
                      spi_host_device_t port = SPI2_HOST, int8_t mode = 0, uint32_t flags = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _data0(data0), _data1(data1), _data2(data2), _data3(data3), _sclk(sclk), _port(port), _mode(mode), _flags(flags)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        bool write(const void *data, size_t byte, uint32_t flags = SPI_TRANS_MODE_QIO) override;
        bool read(void *data, size_t byte, uint32_t flags = SPI_TRANS_MODE_QIO) override;
        bool write_read(const void *write_data, void *read_data, size_t data_byte, uint32_t flags = SPI_TRANS_MODE_QIO) override;
    };
}
