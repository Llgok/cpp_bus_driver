/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-03-14 11:11:19
 * @LastEditTime: 2026-03-28 10:36:04
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define S023MSAFJF10111E1_DEVICE_DEFAULT_ADDRESS_1 0x54

    class S023msafjf10111e1 : public Chip_Iic_Guide
    {
    private:
        enum class Cmd
        {
            RW_INTERNAL_TEST_MODE_REGISTER_CONTROL_1 = 0x0124,

            RW_INTERNAL_TEST_MODE_REGISTER_CONTROL_2 = 0x5128,

            RW_INTERNAL_TEST_MODE = 0x5028,

            RW_DISPLAY_BRIGHTNESS_REGISTER_CONTROL_1 = 0x0124,

            RW_DISPLAY_BRIGHTNESS_REGISTER_CONTROL_2 = 0x0A28,

            RW_DISPLAY_BRIGHTNESS_1 = 0x0328,

            RW_DISPLAY_BRIGHTNESS_2 = 0x0428,

            RW_HORIZONTAL_VERTICAL_MIRROR_1 = 0x032C,

            RW_HORIZONTAL_VERTICAL_MIRROR_2 = 0x042C,
        };

        int32_t _rst;

    public:
        enum class Data_Format
        {
            RGB888,
            INTERNAL_TEST_MODE, // 内部测试图模式
        };

        enum class Internal_Test_Mode
        {
            COLOR_BAR = 0x80,
            BLANK = 0x82,
            WHITE = 0x92,
            RED = 0xA2,
            GREEN = 0xB2,
            BLUE = 0xC2,
        };

        enum class Show_Direction
        {
            NORMAL,
            HORIZONTAL_MIRROR,          // 水平镜像
            VERTICAL_MIRROR,            // 垂直镜像
            HORIZONTAL_VERTICAL_MIRROR, // 水平垂直镜像
        };

        S023msafjf10111e1(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
            : Chip_Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

        /**
         * @brief 设置数据模式
         * @param format 使用Data_Format::配置
         * @return
         * @Date 2026-03-16 10:52:34
         */
        bool set_data_format(Data_Format format);

        /**
         * @brief 内部测试模式
         * @param mode 使用Internal_Test_Mode::配置
         * @return
         * @Date 2026-03-16 10:52:34
         */
        bool set_internal_test_mode(Internal_Test_Mode mode);

        /**
         * @brief 设置显示方向
         * @param direction 使用Show_Direction::配置
         * @return
         * @Date 2026-03-16 10:52:34
         */
        bool set_show_direction(Show_Direction direction);

        /**
         * @brief 设置亮度
         * @param value 值范围：0~511
         * @return
         * @Date 2026-03-16 10:52:34
         */
        bool set_brightness(uint16_t value);
    };
}