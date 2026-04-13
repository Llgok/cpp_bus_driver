/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2026-04-13 17:14:27
 * @License: GPL 3.0
 */
#pragma once

#include "../bus/bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Chip_Iic_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Iic_Guide> _bus;

    private:
        int16_t _address;

    public:
        Chip_Iic_Guide(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address)
            : _bus(bus), _address(address)
        {
        }

        virtual bool begin(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE);
        virtual bool end(void);

        bool init_list(const uint8_t *list, size_t length);
        bool init_list(const uint16_t *list, size_t length);
    };

    class Chip_Spi_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Spi_Guide> _bus;

        int32_t _cs;

    public:
        Chip_Spi_Guide(std::shared_ptr<Bus_Spi_Guide> bus, int32_t cs = CPP_BUS_DRIVER_DEFAULT_VALUE)
            : _bus(bus), _cs(cs)
        {
        }

        virtual bool begin(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE);

        bool init_list(const uint8_t *list, size_t length);
    };

    class Chip_Qspi_Guide : public Tool
    {
    protected:
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
        enum class Spi_Trans
        {
            MODE_DIO = SPI_TRANS_MODE_DIO,
            MODE_QIO = SPI_TRANS_MODE_QIO,
            USE_RXDATA = SPI_TRANS_USE_RXDATA,
            USE_TXDATA = SPI_TRANS_USE_TXDATA,
            MODE_DIOQIO_ADDR = SPI_TRANS_MODE_DIOQIO_ADDR,
            MULTILINE_ADDR = SPI_TRANS_MULTILINE_ADDR,
            VARIABLE_CMD = SPI_TRANS_VARIABLE_CMD,
            VARIABLE_ADDR = SPI_TRANS_VARIABLE_ADDR,
            VARIABLE_DUMMY = SPI_TRANS_VARIABLE_DUMMY,
            CS_KEEP_ACTIVE = SPI_TRANS_CS_KEEP_ACTIVE,
            MULTILINE_CMD = SPI_TRANS_MULTILINE_CMD,
            MODE_OCT = SPI_TRANS_MODE_OCT,
        };
#else
        enum class Spi_Trans
        {
            MODE_DIO,
            MODE_QIO,
            USE_RXDATA,
            USE_TXDATA,
            MODE_DIOQIO_ADDR,
            MULTILINE_ADDR,
            VARIABLE_CMD,
            VARIABLE_ADDR,
            VARIABLE_DUMMY,
            CS_KEEP_ACTIVE,
            MULTILINE_CMD,
            MODE_OCT,
        };
#endif

        std::shared_ptr<Bus_Qspi_Guide> _bus;

        int32_t _cs;

    public:
        Chip_Qspi_Guide(std::shared_ptr<Bus_Qspi_Guide> bus, int32_t cs = CPP_BUS_DRIVER_DEFAULT_VALUE)
            : _bus(bus), _cs(cs)
        {
        }

        virtual bool begin(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE);

        bool init_list(const uint32_t *list, size_t length);
    };

    class Chip_Uart_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Uart_Guide> _bus;

    public:
        Chip_Uart_Guide(std::shared_ptr<Bus_Uart_Guide> bus)
            : _bus(bus)
        {
        }

        virtual bool begin(int32_t baud_rate = CPP_BUS_DRIVER_DEFAULT_VALUE);
    };

    class Chip_Iis_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Iis_Guide> _bus;

    public:
        Chip_Iis_Guide(std::shared_ptr<Bus_Iis_Guide> bus)
            : _bus(bus)
        {
        }

        virtual bool begin(uint16_t mclk_multiple, uint32_t sample_rate_hz, uint8_t data_bit_width) = 0;

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
        bool set_clock_reconfig(uint16_t mclk_multiple, uint32_t sample_rate_hz, Bus_Iis_Guide::Data_Mode data_mode = Bus_Iis_Guide::Data_Mode::INPUT_OUTPUT);
#endif
    };

    class Chip_Sdio_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Sdio_Guide> _bus;

    public:
        Chip_Sdio_Guide(std::shared_ptr<Bus_Sdio_Guide> bus)
            : _bus(bus)
        {
        }

        virtual bool begin(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE);
    };

    class Chip_Mipi_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Mipi_Guide> _bus;

        Init_List_Format _init_list_format;

    public:
        Chip_Mipi_Guide(std::shared_ptr<Bus_Mipi_Guide> bus, Init_List_Format init_list_format = Init_List_Format::WRITE_C8_D8)
            : _bus(bus), _init_list_format(init_list_format)
        {
        }

        virtual bool begin(float freq_mhz = CPP_BUS_DRIVER_DEFAULT_VALUE, float lane_bit_rate_mbps = CPP_BUS_DRIVER_DEFAULT_VALUE);

        bool init_list(const uint8_t *list, size_t length);
    };
}