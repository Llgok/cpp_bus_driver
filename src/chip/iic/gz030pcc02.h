/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-24 13:47:21
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define GZ030PCC02_DEVICE_DEFAULT_ADDRESS 0x28

    class Gz030pcc02 : public Iic_Guide
    {
    private:
        static constexpr uint8_t GZ030PCC02_DEVICE_ID = 0x03; // 默认值

        enum class Cmd
        {
            RO_DEVICE_ID = 0x0001,

        };

        static constexpr const uint16_t _init_list[] =
            {
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00};

        int32_t _rst;

    public:
        Gz030pcc02(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);
    };
}