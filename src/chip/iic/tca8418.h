/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-30 16:56:39
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

            RW_INTERRUPT_STATUS = 0x02,
            RW_KEY_LOCK_AND_EVENT_COUNTER,
            RO_KEY_EVENT,

            WO_GPIO_INT_EN1 = 0x1A, // GPIO中断使能1
            WO_GPIO_INT_EN2,        // GPIO中断使能2
            WO_GPIO_INT_EN3,        // GPIO中断使能3
            WO_KP_GPIO1,            // 按键/GPIO选择1
            WO_KP_GPIO2,            // 按键/GPIO选择2
            WO_KP_GPIO3,            // 按键/GPIO选择3

            WO_GPI_EM1 = 0x20, // GPI事件模式1
            WO_GPI_EM2,        // GPI事件模式2
            WO_GPI_EM3,        // GPI事件模式3
            WO_GPIO_DIR1,      // GPIO数据方向1
            WO_GPIO_DIR2,      // GPIO数据方向2
            WO_GPIO_DIR3,      // GPIO数据方向3
            WO_GPIO_INT_LVL1,  // GPIO边沿/电平检测1
            WO_GPIO_INT_LVL2,  // GPIO边沿/电平检测2
            WO_GPIO_INT_LVL3   // GPIO边沿/电平检测3

        };

        enum class Irq_Flag // 需要清除的中断请求参数，设置1代表清除
        {
            ALL = 0B00011111, // 全部中断
            CTRL_ALT_DEL_KEY_SEQUENCE_FLAG = 0B00010000,
            FIFO_OVERFLOW_FLAG = 0B00001000,
            KEYPAD_LOCK_FLAG = 0B00000100,
            GPIO_INTERRUPT_FLAG = 0B00000010,
            KEY_EVENTS_FLAG = 0B00000001,
        };

        static constexpr const uint8_t _init_list[] =
            {
                //  set default all gpio pins to input
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_DIR1), 0x00,
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_DIR2), 0x00,
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_DIR3), 0x00,

                //  add all pins to key events
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPI_EM1), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPI_EM2), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPI_EM3), 0xFF,

                //  set all pins to falling interrupts
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_LVL1), 0x00,
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_LVL2), 0x00,
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_LVL3), 0x00,

                //  add all pins to interrupts
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_EN1), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_EN2), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_EN3), 0xFF};

        uint8_t _width, _height;
        int32_t _rst;

    public:
        struct Touch_Info
        {
            uint8_t x = -1;                 // x 坐标
            uint8_t y = -1;                 // y 坐标
            bool press_status_flag = false; // 按压状态标志
        };

        struct Touch_Point
        {
            uint8_t finger_count = -1; // 触摸手指总数

            std::vector<struct Touch_Info> info;
        };

        struct Irq_Status // 中断状态
        {
            bool ctrl_alt_del_key_sequence_flag = false; // ctrl+alt+del 这一系统级组合键的同时按下动作标志
            bool fifo_overflow_flag = false;             // fifo溢出中断标志
            bool keypad_lock_flag = false;               // 键盘锁定中断标志
            bool gpio_interrupt_flag = false;            // gpio中断标志
            bool key_events_flag = false;                // 按键事件标志
        };

        Tca8418(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, uint16_t width, uint16_t height, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _width(width), _height(height), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 设置扫描的开窗大小
         * @param x 开窗点x坐标，值范围（0~9）
         * @param y 开窗点y坐标，值范围（0~7）
         * @param w 开窗长度，值范围（0~9）
         * @param h 开窗高度，值范围（0~7）
         * @return
         * @Date 2025-07-30 13:42:08
         */
        bool set_scan_window(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

        /**
         * @brief 获取触摸总数
         * @return
         * @Date 2025-07-30 15:23:03
         */
        uint8_t get_finger_count(void);

        /**
         * @brief 获取多个触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
         * @Date 2025-07-30 16:16:10
         */
        bool get_multiple_touch_point(Touch_Point &tp);

        /**
         * @brief 获取中断标志
         * @return
         * @Date 2025-07-30 16:39:43
         */
        uint8_t get_irq_flag(void);

        /**
         * @brief 中断解析，详细请参考TCA8418手册 8.6.2.2 Interrupt Status Register, INT_STAT (Address 0x02)
         * @param irq_flag 解析状态语句，由get_irq_flag()函数获取
         * @param &status 使用Irq_Status结构体配置，相应位自动置位
         * @return
         * @Date 2025-07-30 16:39:56
         */
        bool parse_irq_status(uint8_t irq_flag, Irq_Status &status);

        /**
         * @brief 清除中断标志位
         * @param flag 使用Irq_Flag::配置，需要清除的标志，设置1为清除标志
         * @return
         * @Date 2025-07-30 16:55:59
         */
        bool clear_irq_flag(Irq_Flag flag);
    };
}