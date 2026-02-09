/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-02-03 15:06:34
 * @LastEditTime: 2026-02-09 09:44:38
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define AXP517_DEVICE_DEFAULT_ADDRESS 0x34

    class Axp517 : public Chip_Iic_Guide
    {
    private:
        static constexpr uint8_t DEVICE_ID = 0x02;

        enum class Cmd
        {
            RO_DEVICE_ID = 0xCE,

            // 状态寄存器
            RO_BMU_STATUS0 = 0x00, // BMU状态0
            RO_BMU_STATUS1,        // BMU状态1

            RO_BC_DETECT = 0x05, // BC检测结果
            RO_BMU_FAULT0,       // BMU故障0

            RO_BMU_FAULT1 = 0x08, // BMU故障1

            // 模块使能控制
            RW_MODULE_ENABLE_CONTROL0 = 0x0B, // 模块使能控制0

            // 通用配置
            RW_COMMON_CONFIGURE = 0x10, // 通用配置
            RW_GPIO_CONFIGURE,          // GPIO配置
            RW_BATFET_CONTROL,          // BATFET控制
            RW_RBFET_CONTROL,           // RBFET控制

            // 充电相关
            RW_MINIMUM_SYSTEM_VOLTAGE_CONTROL = 0x15, // 最小系统电压控制
            RW_INPUT_VOLTAGE_LIMIT_CONTROL,           // 输入电压限制控制
            RW_INPUT_CURRENT_LIMIT_CONTROL,           // 输入电流限制控制

            RW_MODULE_ENABLE_CONTROL1 = 0x19, // 模块使能控制1
            RW_WATCHDOG_CONTROL,              // 看门狗控制

            RW_BOOST_CONFIGURE = 0x1E, // Boost配置

            // 温度传感器
            RW_TS_PIN_CONFIGURE = 0x50, // TS引脚配置

            RW_VLTF_CHG_SETTING = 0x54, // 充电低温阈值设置
            RW_VHTF_CHG_SETTING,        // 充电高温阈值设置
            RW_VLTF_WORK_SETTING,       // 工作低温阈值设置
            RW_VHTF_WORK_SETTING,       // 工作高温阈值设置

            // JEITA标准
            RW_JEITA_STANDARD_ENABLE_CONTROL = 0x58, // JEITA标准使能控制
            RW_JEITA_CURRENT_VOLTAGE_CONFIGURATION,  // JEITA电流/电压配置

            // 充电控制
            RW_IPRECHG_ITRICHG_SETTING = 0x61,       // 预充电/涓流充电设置
            RW_ICC_SETTING,                          // 恒流充电设置
            RW_ITERM_SETTING_AND_CONTROL,            // 终止电流设置和控制
            RW_CV_CHARGER_VOLTAGE_SETTING,           // 恒压充电电压设置
            RW_THERMAL_REGULATION_THRESHOLD_SETTING, // 热调节阈值设置

            RW_CHARGER_TIMER_CONFIGURE = 0x67, // 充电定时器配置

            // 电量计
            RW_FUEL_GAUGE_CONTROL = 0x71, // 电量计控制
            RO_BATTERY_TEMPERATURE,       // 电池温度
            RO_BATTERY_SOH,               // 电池健康度
            RO_BATTERY_PERCENTAGE,        // 电池百分比

            // ADC相关
            RW_ADC_CHANNEL_ENABLE_CONTROL = 0x90, // ADC通道使能控制
            RO_VBAT_H,                            // 电池电压高字节
            RO_VBAT_L,                            // 电池电压低字节
            RO_IBAT_H,                            // 电池电流高字节
            RO_IBAT_L,                            // 电池电流低字节
            RO_TS_H,                              // TS电压高字节
            RO_TS_L,                              // TS电压低字节
            RO_VBUS_CURRENT_H,                    // VBUS电流高字节
            RO_VBUS_CURRENT_L,                    // VBUS电流低字节
            RO_VBUS_VOLTAGE_H,                    // VBUS电压高字节
            RO_VBUS_VOLTAGE_L,                    // VBUS电压低字节
            RW_ADC_DATA_SELECT,                   // ADC数据选择
            RO_ADC_DATA_H,                        // ADC数据高字节
            RO_ADC_DATA_L,                        // ADC数据低字节

            // 中断
            RW_IRQ_ENABLE0 = 0x40, // 中断使能0
            RW_IRQ_ENABLE1,        // 中断使能1
            RW_IRQ_ENABLE2,        // 中断使能2
            RW_IRQ_ENABLE3,        // 中断使能3

            RW_IRQ_STATUS0 = 0x48, // 中断状态0
            RW_IRQ_STATUS1,        // 中断状态1
            RW_IRQ_STATUS2,        // 中断状态2
            RW_IRQ_STATUS3,        // 中断状态3

            // PD相关
            RW_TCPC_CONTROL = 0xB9, // TCPC控制
            RW_ROLE_CONTROL,        // 角色控制
            RW_COMMAND = 0xC3,      // 命令寄存器
        };

        static constexpr const uint8_t _init_list[] =
            {
                static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_INPUT_CURRENT_LIMIT_CONTROL), 0B11111100, // 输入电流限制修改为最大

                static_cast<uint8_t>(Init_List_Format::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_ICC_SETTING), 0B00001000 // 设置充电电流为512mA
            };

        int32_t _rst;
        int32_t _irq;

    public:
        enum class Charge_Status
        {
            TRICKLE_CHARGE,
            PRECHARGE,
            CONSTANT_CURRENT,
            CONSTANT_VOLTAGE,
            CHARGE_DONE,
            NOT_CHARGING,
            INVALID,
        };

        enum class Battery_Current_Direction
        {
            STANDBY,
            CHARGE,
            DISCHARGE,
            INVALID,
        };

        enum class Ntc_Fault_Status
        {
            NORMAL = 0,
            TS_COLD_CHARGE = 1,
            TS_HOT_CHARGE = 2,
            TS_COLD_WORK = 5,
            TS_HOT_WORK = 6,
        };

        enum class Bc_Detect_Result
        {
            SDP = 1, // 标准下行端口
            CDP = 2, // 充电下行端口
            DCP = 3, // 专用充电端口
        };

        // struct Irq_Status_0
        // {
        //     bool vbus_fault_flag = false;
        //     bool vbus_over_voltage_flag = false;
        //     bool boost_over_voltage_flag = false;
        //     bool charge_to_normal_flag = false;
        //     bool gauge_new_soc_flag = false;
        //     bool soc_drop_to_shutdown_level_flag = false;
        //     bool soc_drop_to_warning_level_flag = false;
        // };

        // struct Irq_Status_1
        // {
        //     bool pwr_on_positive_edge_flag = false;
        //     bool pwr_on_negative_edge_flag = false;
        //     bool pwr_on_long_press_flag = false;
        //     bool pwr_on_short_press_flag = false;
        //     bool battery_remove_flag = false;
        //     bool battery_insert_flag = false;
        //     bool vbus_remove_flag = false;
        //     bool vbus_insert_flag = false;
        // };

        // struct Irq_Status_2
        // {
        //     bool battery_over_voltage_flag = false;
        //     bool charger_safety_timer_expire_flag = false;
        //     bool die_over_temperature_level1_flag = false;
        //     bool charger_start_flag = false;
        //     bool battery_charge_done_flag = false;
        //     bool batfet_over_current_flag = false;
        //     bool watchdog_expire_flag = false;
        // };

        // struct Irq_Status_3
        // {
        //     bool battery_under_temperature_work_flag = false;
        //     bool battery_over_temperature_work_flag = false;
        //     bool battery_under_temperature_charge_flag = false;
        //     bool battery_over_temperature_charge_flag = false;
        //     bool battery_over_temperature_quit_flag = false;
        //     bool bc1_2_detect_result_change_flag = false;
        //     bool bc1_2_detect_finished_flag = false;
        // };

        enum class Adc_Data
        {
            CHIP_TEMPERATURE_CELSIUS = 0,
            SYSTEM_VOLTAGE,

            CHARGING_CURRENT = 6,
            DISCHARGE_CURRENT,
        };

        enum class Gpio_Source
        {
            BY_REG = 0, // 通过寄存器控制
            PD_IRQ,     // PD_IRQ
        };

        enum class Gpio_Mode
        {
            INPUT,  // 输入模式
            OUTPUT, // 输出模式
        };

        enum class Gpio_Status
        {
            HIZ = 0, // 高阻态
            LOW,     // 低电平
            HIGH,    // 高电平
            INVALID, // 无效
        };

        enum class Force_Batfet
        {
            AUTO,
            ON,
            OFF,
        };

        struct Chip_Status_0
        {
            bool current_limit_status = false;
            bool thermal_regulation_status = false;
            bool battery_in_active_mode = false;
            bool battery_present_status = false;
            bool batfet_status = false;
            bool vbus_good_indication = false;
        };

        struct Chip_Status_1
        {
            Charge_Status charging_status = Charge_Status::NOT_CHARGING;
            bool vindpm_status = false;
            bool system_status_indication = false;
            Battery_Current_Direction battery_current_direction = Battery_Current_Direction::STANDBY;
        };

        struct Adc_Channel
        {
            bool vbus_current_measure = false;
            bool battery_discharge_current_measure = false;
            bool battery_charge_current_measure = false;
            bool chip_temperature_measure = false;
            bool system_voltage_measure = false;
            bool vbus_voltage_measure = false;
            bool ts_value_measure = false;
            bool battery_voltage_measure = false;
        };

        Axp517(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE, int32_t irq = CPP_BUS_DRIVER_DEFAULT_VALUE)
            : Chip_Iic_Guide(bus, address), _rst(rst), _irq(irq)
        {
        }

        bool begin(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
        bool end() override;

        uint8_t get_device_id(void);

        /**
         * @brief 获取芯片状态0
         * @param &status 使用Chip_Status_0::配置
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool get_chip_status_0(Chip_Status_0 &status);

        /**
         * @brief 获取芯片状态1
         * @param &status 使用Chip_Status_1::配置
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool get_chip_status_1(Chip_Status_1 &status);

        // /**
        //  * @brief 获取所有中断状态
        //  * @param &status0 中断状态0
        //  * @param &status1 中断状态1
        //  * @param &status2 中断状态2
        //  * @param &status3 中断状态3
        //  * @return
        //  * @Date 2026-02-03 15:06:34
        //  */
        // bool get_irq_status(Irq_Status_0 &status0, Irq_Status_1 &status1, Irq_Status_2 &status2, Irq_Status_3 &status3);

        // /**
        //  * @brief 清除所有中断标志
        //  * @return
        //  * @Date 2026-02-03 15:06:34
        //  */
        // bool clear_all_irq(void);

        /**
         * @brief 设置充电使能
         * @param enable [true]：开启充电 [false]：关闭充电
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_charge_enable(bool enable);

        /**
         * @brief 设置充电电流
         * @param current_ma 充电电流值(mA)，范围0-5120mA，64mA/步进
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_charge_current(uint16_t current_ma);

        /**
         * @brief 设置充电电压
         * @param voltage_mv 充电电压值(mV)，支持4000, 4100, 4200, 4350, 4400, 3800, 3600, 5000mV
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_charge_voltage(uint16_t voltage_mv);

        /**
         * @brief 设置输入电流限制
         * @param limit_ma 输入电流限制值(mA)，范围100-3250mA，50mA/步进
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_input_current_limit(uint16_t limit_ma);

        /**
         * @brief 设置输入电压限制
         * @param limit_mv 输入电压限制值(mV)，范围3600-16200mV，100mV/步进
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_input_voltage_limit(uint16_t limit_mv);

        /**
         * @brief 获取电池电量百分比
         * @return 电池电量百分比(0-100)
         * @Date 2026-02-03 15:06:34
         */
        uint8_t get_battery_level(void);

        /**
         * @brief 获取电池健康度
         * @return 电池健康度(0-100)int16_t
         * @Date 2026-02-03 15:06:34
         */
        uint8_t get_battery_health(void);

        /**
         * @brief 获取电池温度，使用前需要开启对应ADC通道（REG 90H）
         * @return 电池温度(℃)
         * @Date 2026-02-03 15:06:34
         */
        int8_t get_battery_temperature_celsius(void);

        /**
         * @brief 设置ADC通道
         * @param channel 使用Adc_Channel::配置
         * @return
         * @Date 2026-02-04 10:16:10
         */
        bool set_adc_channel(Adc_Channel channel);

        /**
         * @brief 获取电池电压，使用前需要开启对应ADC通道（REG 90H）
         * @return 电池电压(mV)
         * @Date 2026-02-03 15:06:34
         */
        uint16_t get_battery_voltage(void);

        /**
         * @brief 获取电池电流，使用前需要开启对应ADC通道（REG 90H）
         * @return 电池电流(mA)，正值为充电，负值为放电，读取失败返回-32768
         * @Date 2026-02-03 15:06:34
         */
        float get_battery_current(void);

        /**
         * @brief 获取TS引脚电压值，使用前需要开启对应ADC通道（REG 90H）
         * @return TS引脚电压值(mV)
         * @Date 2026-02-03 15:06:34
         */
        float get_ts_voltage(void);

        /**
         * @brief 获取VBUS电流，使用前需要开启对应ADC通道（REG 90H）
         * @return VBUS电压(mA)
         * @Date 2026-02-03 15:06:34
         */
        uint16_t get_vbus_current(void);

        /**
         * @brief 获取VBUS电压，使用前需要开启对应ADC通道（REG 90H）
         * @return VBUS电压(mV)
         * @Date 2026-02-03 15:06:34
         */
        uint16_t get_vbus_voltage(void);

        /**
         * @brief 设置adc数据输出选择
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_adc_data_select(Adc_Data data_select);

        /**
         * @brief 获取ADC数据值
         * @return
         * @Date 2026-02-03 15:06:34
         */
        uint16_t get_adc_data(void);

        /**
         * @brief 获取芯片结温温度，使用前需要开启ADC数据选择（REG 9BH）中的Tdie和对应ADC通道（REG 90H）
         * @return 芯片结温温度(℃)
         * @Date 2026-02-03 15:06:34
         */
        float get_chip_die_junction_temperature_celsius(void);

        /**
         * @brief 获取系统电压，使用前需要开启ADC数据选择（REG 9BH）中的Vsys和对应ADC通道（REG 90H）
         * @return 系统电压(mV)
         * @Date 2026-02-03 15:06:34
         */
        uint16_t get_system_voltage(void);

        /**
         * @brief 获取充电电流，使用前需要开启ADC数据选择（REG 9BH）中的Ichg和对应ADC通道（REG 90H）
         * @return 充电电流(mV)
         * @Date 2026-02-03 15:06:34
         */
        float get_charging_current(void);

        /**
         * @brief 获取放电电流，使用前需要开启ADC数据选择（REG 9BH）中的Idischg和对应ADC通道（REG 90H）
         * @return 放电电流(mV)
         * @Date 2026-02-03 15:06:34
         */
        float get_discharging_current(void);

        /**
         * @brief 设置Boost模式使能
         * @param enable [true]：开启Boost模式 [false]：关闭Boost模式
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_boost_enable(bool enable);

        /**
         * @brief 设置GPIO输出源选择
         * @param source 使用Gpio_Source::配置
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_gpio_source(Gpio_Source source);

        /**
         * @brief 设置GPIO模式
         * @param mode 使用Gpio_Mode::配置
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_gpio_mode(Gpio_Mode mode);

        /**
         * @brief 写GPIO状态
         * @param config 使用Gpio_Status::配置
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool gpio_write(Gpio_Status status);

        /**
         * @brief 读取GPIO状态
         * @return 使用Gpio_Status::配置
         * @Date 2026-02-03 15:06:34
         */
        Gpio_Status gpio_read(void);

        /**
         * @brief 设置开启运输模式
         * @param enable [true]：开启运输模式 [false]：关闭运输模式
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_shipping_mode_enable(bool enable);

        /**
         * @brief 强制设置batfet（电池开关）启动或者关闭
         * @param mode 使用Force_Batfet::配置
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_force_batfet_mode(Force_Batfet mode);

        /**
         * @brief 强制设置rbfet（vbus反向供电开关）启动或者关闭
         * @param enable enable [true]：强制开启 [false]：强制关闭
         * @return
         * @Date 2026-02-03 15:06:34
         */
        bool set_force_rbfet_enable(bool enable);

        // /**
        //  * @brief 设置Boost输出电压
        //  * @param voltage_mv 输出电压(mV)，范围4550-5510mV，64mV/步进
        //  * @return
        //  * @Date 2026-02-03 15:06:34
        //  */
        // bool set_boost_voltage(uint16_t voltage_mv);

        // /**
        //  * @brief 设置看门狗
        //  * @param enable [true]：开启看门狗 [false]：关闭看门狗
        //  * @param timeout_s 超时时间(秒)，支持1,2,4,8,16,32,64,128秒
        //  * @return
        //  * @Date 2026-02-03 15:06:34
        //  */
        // bool set_watchdog(bool enable, uint8_t timeout_s);

        // /**
        //  * @brief 喂狗
        //  * @return
        //  * @Date 2026-02-03 15:06:34
        //  */
        // bool feed_watchdog(void);

        // /**
        //  * @brief 设置JEITA标准使能
        //  * @param enable [true]：开启JEITA标准 [false]：关闭JEITA标准
        //  * @return
        //  * @Date 2026-02-03 15:06:34
        //  */
        // bool set_jeita_enable(bool enable);

        // /**
        //  * @brief 设置BC1.2检测使能
        //  * @param enable [true]：开启BC1.2检测 [false]：关闭BC1.2检测
        //  * @return
        //  * @Date 2026-02-03 15:06:34
        //  */
        // bool set_bc12_detect_enable(bool enable);

        // /**
        //  * @brief 获取BC1.2检测结果
        //  * @param &result BC检测结果
        //  * @return
        //  * @Date 2026-02-03 15:06:34
        //  */
        // bool get_bc12_detect_result(Bc_Detect_Result &result);

        // /**
        //  * @brief 设置PD角色
        //  * @param is_source [true]：源模式 [false]：汇模式
        //  * @param is_drp [true]：双角色模式 [false]：固定角色
        //  * @return
        //  * @Date 2026-02-03 15:06:34
        //  */
        // bool set_pd_role(bool is_source, bool is_drp);
    };
}