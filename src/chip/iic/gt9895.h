/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-08 14:41:57
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define GT9895_TOUCH_DEVICE_DEFAULT_ADDRESS_1 0x5D

    class Gt9895 : public Iic_Guide
    {
    private:
        static constexpr uint8_t MAX_TOUCH_FINGER_COUNT = 10;

        static constexpr uint8_t TOUCH_POINT_ADDRESS_OFFSET = 10;
        static constexpr uint8_t SINGLE_TOUCH_POINT_DATA_SIZE = 8;

        enum class Cmd
        {
            // 触摸信息开始地址
            RO_TOUCH_INFO_START_ADDRESS = 0x00010308,
        };

        // static constexpr uint8_t Init_List[];

        int32_t _rst;

    public:
        struct Touch_Info
        {
            uint16_t x = -1;             // X 坐标
            uint16_t y = -1;             // Y 坐标
            uint8_t pressure_value = -1; // 触摸压力值
        };

        struct Touch_Point
        {
            uint8_t finger_count = -1;    // 触摸手指总数
            uint8_t finger_id = -1;       // 触摸手指id
            bool edge_touch_flag = false; // 边缘触摸标志

            std::vector<struct Touch_Info> info;
        };

        Gt9895(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        /**
         * @brief 获取触摸总数
         * @return
         * @Date 2025-03-28 09:51:25
         */
        uint8_t get_finger_count(void);

        /**
         * @brief 获取单指触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @param finger_num 要获取的触摸点
         * @return [true]：获取的触摸点和finger_num相同 [false]：获取错误或者获取的触摸点和finger_num不相同
         * @Date 2025-03-28 09:49:03
         */
        bool get_single_touch_point(Touch_Point &tp, uint8_t finger_num = 1);

        /**
         * @brief 获取多个触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
         * @Date 2025-03-28 09:52:56
         */
        bool get_multiple_touch_point(Touch_Point &tp);

        /**
         * @brief 获取边缘检测
         * @return  [true]：屏幕边缘检测触发 [false]：屏幕边缘检测未触发
         * @Date 2025-03-28 09:56:59
         */
        bool get_edge_touch(void);
    };
}