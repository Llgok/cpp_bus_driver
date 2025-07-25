/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-24 14:43:26
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define SGM38121_DEVICE_DEFAULT_ADDRESS 0x28

    class Sgm38121 : public Iic_Guide
    {
    private:
        static constexpr uint8_t DEVICE_ID = 0x80;

        enum class Cmd
        {
            RO_DEVICE_ID = 0x00,

            RW_DISCHARGE_RESISTOR_SELECTION = 0X02,
            RW_DVDD1_OUTPUT_VOLTAGE_LEVEL,
            RW_DVDD2_OUTPUT_VOLTAGE_LEVEL,
            RW_AVDD1_OUTPUT_VOLTAGE_LEVEL,
            RW_AVDD2_OUTPUT_VOLTAGE_LEVEL,
            RW_FUNCTION,
            RW_POWER_SEQUENCE_SETTING_1 = 0X0A,
            RW_POWER_SEQUENCE_SETTING_2,
            RW_ENABLE_CONTROL = 0X0E,
            RW_SEQUENCE_CONTROL,
        };

        int32_t _rst;

    public:
        enum class Channel
        {
            DVDD_1 = 0,
            DVDD_2,
            AVDD_1,
            AVDD_2,
        };

        enum class Status
        {
            ON = 0,
            OFF,
        };

        Sgm38121(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 设置输出电压
         * @param channel 使用Channel::配置
         * @param voltage DVDD_1和DVDD_2取值528~1504，AVDD_1和AVDD_2取值1504~3424
         * @return 
         * @Date 2025-07-17 10:27:58
         */        
        bool set_output_voltage(Channel channel, uint16_t voltage);

        /**
         * @brief 设置通道状态
         * @param channel 使用Channel::配置
         * @param status 使用Status::配置
         * @return 
         * @Date 2025-07-17 10:29:45
         */        
        bool set_channel_status(Channel channel, Status status);
    };
}