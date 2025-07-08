/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-07-08 14:43:48
 * @License: GPL 3.0
 */
#include "gt9895.h"

namespace Cpp_Bus_Driver
{
    // constexpr uint8_t Gt9895::Init_List[] =
    //     {
    //         static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8),
    //         static_cast<uint8_t>(Cmd::RW_CLKOUT_CONTROL),
    //         0B00000000,
    // };

    bool Gt9895::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        return true;
    }

    // uint8_t Gt9895::device_id(void)
    // {
    //     uint8_t buffer = 0;

    //     if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return -1;
    //     }

    //     return buffer;
    // }

    uint8_t Gt9895::get_finger_count(void)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 24),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 8),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS)),
            };

        uint8_t buffer_2[3] = {0};

        if (_bus->write_read(buffer, 4, buffer_2, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return -1;
        }

        return buffer_2[2];
    }

    bool Gt9895::get_single_touch_point(Touch_Point &tp, uint8_t finger_num)
    {
        if ((finger_num == 0) || (finger_num > MAX_TOUCH_FINGER_COUNT))
        {
            return false;
        }

        uint8_t buffer[] =
            {
                static_cast<uint8_t>((static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) + TOUCH_POINT_ADDRESS_OFFSET + (finger_num - 1) * SINGLE_TOUCH_POINT_DATA_SIZE) >> 24),
                static_cast<uint8_t>((static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) + TOUCH_POINT_ADDRESS_OFFSET + (finger_num - 1) * SINGLE_TOUCH_POINT_DATA_SIZE) >> 16),
                static_cast<uint8_t>((static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) + TOUCH_POINT_ADDRESS_OFFSET + (finger_num - 1) * SINGLE_TOUCH_POINT_DATA_SIZE) >> 8),
                static_cast<uint8_t>((static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) + TOUCH_POINT_ADDRESS_OFFSET + (finger_num - 1) * SINGLE_TOUCH_POINT_DATA_SIZE)),
            };

        uint8_t buffer_2[SINGLE_TOUCH_POINT_DATA_SIZE] = {0};

        if (_bus->write_read(buffer, 4, buffer_2, SINGLE_TOUCH_POINT_DATA_SIZE) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        uint16_t buffer_x = (buffer_2[2] | static_cast<uint16_t>(buffer_2[3]) << 8);
        uint16_t buffer_y = (buffer_2[4] | static_cast<uint16_t>(buffer_2[5]) << 8);

        if ((buffer_x == static_cast<uint16_t>(-1)) && (buffer_y == static_cast<uint16_t>(-1)))
        {
            return false;
        }

        tp.finger_count = finger_num;

        Touch_Info buffer_ti;
        buffer_ti.x = buffer_x;
        buffer_ti.y = buffer_y;
        buffer_ti.pressure_value = buffer_2[6];

        tp.info.push_back(buffer_ti);

        return true;
    }

    bool Gt9895::get_multiple_touch_point(Touch_Point &tp)
    {
        uint8_t buffer[] =
            {
                0xF3,
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 24),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 8),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS)),
                0x03,
            };

        const uint8_t buffer_touch_point_size = TOUCH_POINT_ADDRESS_OFFSET + MAX_TOUCH_FINGER_COUNT * SINGLE_TOUCH_POINT_DATA_SIZE;
        uint8_t buffer_2[buffer_touch_point_size] = {0};

        if (_bus->write_read(buffer, 6, buffer_2, buffer_touch_point_size) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        // 如果手指数为0或者大于最大触摸手指数
        if ((buffer_2[0] == 0) || (buffer_2[0] > MAX_TOUCH_FINGER_COUNT))
        {
            return false;
        }
        tp.finger_count = buffer_2[0];

        for (uint8_t i = 1; i <= tp.finger_count; i++)
        {
            const uint8_t buffer_touch_point_offset = TOUCH_POINT_ADDRESS_OFFSET + i * SINGLE_TOUCH_POINT_DATA_SIZE - SINGLE_TOUCH_POINT_DATA_SIZE;

            Touch_Info buffer_ti;
            buffer_ti.x = (static_cast<uint16_t>(buffer_2[buffer_touch_point_offset]) << 8) | buffer_2[buffer_touch_point_offset + 1];
            buffer_ti.y = (static_cast<uint16_t>(buffer_2[buffer_touch_point_offset + 2]) << 8) | buffer_2[buffer_touch_point_offset + 3];
            buffer_ti.pressure_value = buffer_2[buffer_touch_point_offset + 4];

            tp.info.push_back(buffer_ti);
        }

        if ((tp.info[tp.finger_count - 1].x == static_cast<uint16_t>(-1)) &&
            (tp.info[tp.finger_count - 1].y == static_cast<uint16_t>(-1)) &&
            (tp.info[tp.finger_count - 1].pressure_value == 0))
        {
            tp.edge_touch_flag = true;
        }
        else
        {
            tp.edge_touch_flag = false;
        }

        return true;
    }

    bool Gt9895::get_edge_touch(void)
    {
        uint8_t buffer[] =
            {
                0xF3,
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 24),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 8),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS)),
                0x03,
            };

        const uint8_t buffer_touch_point_size = TOUCH_POINT_ADDRESS_OFFSET + MAX_TOUCH_FINGER_COUNT * SINGLE_TOUCH_POINT_DATA_SIZE;
        uint8_t buffer_2[buffer_touch_point_size] = {0};

        if (_bus->write_read(buffer, 6, buffer_2, buffer_touch_point_size) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        // 如果手指数为0
        if (buffer_2[0] == 0)
        {
            return false;
        }

        const uint8_t buffer_touch_point_offset = TOUCH_POINT_ADDRESS_OFFSET + buffer_2[0] * SINGLE_TOUCH_POINT_DATA_SIZE - SINGLE_TOUCH_POINT_DATA_SIZE;

        if ((static_cast<uint16_t>((static_cast<uint16_t>(buffer_2[buffer_touch_point_offset]) << 8) | buffer_2[buffer_touch_point_offset + 1]) != static_cast<uint16_t>(-1)) ||
            (static_cast<uint16_t>((static_cast<uint16_t>(buffer_2[buffer_touch_point_offset + 2]) << 8) | buffer_2[buffer_touch_point_offset + 3]) != static_cast<uint16_t>(-1)) ||
            (buffer_2[buffer_touch_point_offset + 4] != 0))
        {
            return false;
        }

        return true;
    }

}
