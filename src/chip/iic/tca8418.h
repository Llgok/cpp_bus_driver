/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-30 11:18:15
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
        static constexpr uint8_t DEVICE_ID = 0xC4;

        enum class Cmd
        {
            RO_DEVICE_ID = 0x2F,

            WO_GPIO_INT_EN1 = 0x1A, // gpio interrupt enable 1
            WO_GPIO_INT_EN2,        // gpio interrupt enable 2
            WO_GPIO_INT_EN3,        // gpio interrupt enable 3

            WO_GPI_EM1 = 0x20, // gpi event mode 1
            WO_GPI_EM2,        // gpi event mode 2
            WO_GPI_EM3,        // gpi event mode 3
            WO_GPIO_DIR1,      // gpio data direction 1
            WO_GPIO_DIR2,      // gpio data direction 2
            WO_GPIO_DIR3,      // gpio data direction 3
            WO_GPIO_INT_LVL1,  // gpio edge/level detect 1
            WO_GPIO_INT_LVL2,  // gpio edge/level detect 2
            WO_GPIO_INT_LVL3,  // gpio edge/level detect 3

        };

        static constexpr const uint8_t _init_list[] =
            {
                //  set default all gio pins to input
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_DIR1), 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_DIR2), 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_DIR3), 0x00,

                //  add all pins to key events
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPI_EM1), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPI_EM2), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPI_EM3), 0xFF,

                //  set all pins to falling interrupts
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_LVL1), 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_LVL2), 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_LVL3), 0x00,

                //  add all pins to interrupts
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_EN1), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_EN2), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_EN3), 0xFF};

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