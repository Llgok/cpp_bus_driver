/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-01-24 15:47:38
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
    class Rm69a10 : public Mipi_Guide
    {
    private:
        static constexpr uint8_t DEVICE_ID = 0x01;

        enum class Cmd
        {
            RO_DEVICE_ID = 0xA1,

            WO_SLPIN = 0x10,
            WO_SLPOUT,

            WO_INVOFF = 0x20,
            WO_INVON,

            WO_WRDISBV = 0x51,

            WO_DISPOFF = 0x28,
            WO_DISPON,
        };

        static constexpr const uint8_t _init_list[] =
            {
                static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8), 0xFE, 0xFD,
                static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8), 0x80, 0xFC,
                static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8), 0xFE, 0x00,

                static_cast<uint8_t>(Init_List_Format::WRITE_C8_BYTE_DATA), 0x2A, 4,
                0x00, 0x00, 0x02, 0x37,

                static_cast<uint8_t>(Init_List_Format::WRITE_C8_BYTE_DATA), 0x2B, 4,
                0x00, 0x00, 0x04, 0xCF,

                static_cast<uint8_t>(Init_List_Format::WRITE_C8_BYTE_DATA), 0x31, 4,
                0x00, 0x03, 0x02, 0x34,

                static_cast<uint8_t>(Init_List_Format::WRITE_C8_BYTE_DATA), 0x30, 4,
                0x00, 0x00, 0x04, 0xCF,

                static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8), 0x12, 0x00,
                static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8), 0x35, 0x00,

                static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8), 0x51, 0x00, // 设置屏幕亮度为0

                static_cast<uint8_t>(Init_List_Format::WRITE_C8), 0x11,
                static_cast<uint8_t>(Init_List_Format::DELAY_MS), 120,

                static_cast<uint8_t>(Init_List_Format::WRITE_C8), 0x29};

        int32_t _rst;

        uint8_t _madctl_data = 0;

    public:
        Rm69a10(std::shared_ptr<Bus_Mipi_Guide> bus, int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
            : Mipi_Guide(bus, Init_List_Format::WRITE_C8_D8), _rst(rst)
        {
        }

        bool begin(int32_t freq_mhz = CPP_BUS_DRIVER_DEFAULT_VALUE, int32_t lane_bit_rate_mbps = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 设置睡眠
         * @param enable [true]：进入睡眠 [false]：退出睡眠
         * @return
         * @Date 2026-01-24 10:37:09
         */
        bool set_sleep(bool enable);

        /**
         * @brief 设置屏幕关闭
         * @param enable [true]：关闭屏幕 [false]：开启屏幕
         * @return
         * @Date 2026-01-24 10:37:09
         */
        bool set_screen_off(bool enable);

        /**
         * @brief 设置颜色反转
         * @param enable [true]：开启颜色反转 [false]：关闭颜色反转
         * @return
         * @Date 2026-01-24 10:37:09
         */
        bool set_inversion(bool enable);

        /**
         * @brief 设置屏幕亮度
         * @param brightness 亮度值 [0-255]，0最暗，255最亮
         * @return
         * @Date 2026-01-24 10:37:09
         */
        bool set_brightness(uint8_t brightness);

        /**
         * @brief 发送色彩流以坐标的形式
         * @param x_start x坐标开始点
         * @param x_end x坐标结束点
         * @param y_start y坐标开始点
         * @param y_end x坐标结束点
         * @param *data 颜色数据
         * @return
         * @Date 2026-01-24 17:06:45
         */
        bool send_color_stream_coordinate(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, const uint8_t *data);
    };
};
