/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-30 10:40:38
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define TCA8418_DEVICE_DEFAULT_ADDRESS 0x34

    class Tca8418 : public Iic_Guide
    {
    private:
        static constexpr uint8_t DEVICE_ID = 0x80;

        enum class Cmd
        {
            RO_DEVICE_ID = 0x2F,

        };

        int32_t _rst;

    public:
        Tca8418(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);
    };
}