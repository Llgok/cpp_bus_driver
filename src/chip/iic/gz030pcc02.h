/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-24 15:27:14
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
        static constexpr uint8_t DEVICE_ID = 0x03; // 默认值

        static constexpr uint8_t MIPI_LANE_NUM = 2; // MIPI 总线的 lane 数量

        enum class Cmd
        {
            RO_DEVICE_ID = 0x0001,

        };

        static constexpr const uint16_t _init_list[] =
            {
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x08,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x00,
#if MIPI_LANE_NUM == 4
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x03,
#elif MIPI_LANE_NUM == 2
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x01,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x5F00, 0x22,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x9F00, 0x06,
#endif
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x10,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x07,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x10,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x03,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x0F,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x14,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x03,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x02,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x01,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x08,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x11,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x01,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x7D02, 0xC0,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x7E03, 0x01,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6F00, 0x30,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x7402, 0x0D,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x9F01, 0x10};

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