/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-17 12:01:26
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define SGM41562XX_DEVICE_DEFAULT_ADDRESS 0x03

    class Sgm41562xx : public Iic_Guide
    {
    private:
        static constexpr uint8_t SGM41562XX_DEVICE_ID = 0x04;

        enum class Cmd
        {
            RO_DEVICE_ID = 0x0B,

            RW_INPUT_SOURCE_CONTROL = 0x00,             // 输入源控制寄存器
            RW_POWER_ON_CONFIGURATION,                  // 上电配置寄存器
            RW_CHARGE_CURRENT_CONTROL,                  // 充电电流控制寄存器
            RW_DISCHARGE_TERMINATION_CURRENT,           // 放电/终止电流寄存器
            RW_CHARGE_VOLTAGE_CONTROL,                  // 充电电压控制寄存器
            RW_CHARGE_TERMINATION_TIMER_CONTROL,        // 充电终止/定时器控制寄存器
            RW_MISCELLANEOUS_OPERATION_CONTROL,         // 杂项操作控制寄存器
            RW_SYSTEM_VOLTAGE_REGULATION,               // 系统电压调节寄存器
            RD_SYSTEM_STATUS,                           // 系统状态寄存器
            RD_FAULT,                                   // 故障寄存器
            RW_IIC_ADDRESS_MISCELLANEOUS_CONFIGURATION, // IIC地址及杂项配置寄存器
        };

        static constexpr const uint8_t _init_list[] =
            {
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_CHARGE_CURRENT_CONTROL), 0B11001111, // 重置寄存器
                // static_cast<uint8_t>(Init_List_Cmd::DELAY_MS), 120,
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), 0B01011111, // 关闭NTC 屏蔽INT
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_IIC_ADDRESS_MISCELLANEOUS_CONFIGURATION), 0B01100001, // 充电电流权重限制

                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), 0B00011010, // 关闭看门狗功能
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), 0B10100100,           // 开启电池充电功能

                //  static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), 0B10101100,        // 关闭电池充电功能

                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), 0B01000000, // 关闭输入电流限制

                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), 0B00100000, // 添加200ma电流阈值到输入电流限制中（仅在电流限制模式有效）

            };

        int32_t _rst;

    public:
        Sgm41562xx(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);
    };
}