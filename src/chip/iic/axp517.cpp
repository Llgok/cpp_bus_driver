/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-02-03 15:06:34
 * @LastEditTime: 2026-02-09 09:47:25
 * @License: GPL 3.0
 */
#include "axp517.h"

namespace Cpp_Bus_Driver
{
    bool Axp517::begin(int32_t freq_hz)
    {
        if (_rst != CPP_BUS_DRIVER_DEFAULT_VALUE)
        {
            pin_mode(_rst, Pin_Mode::OUTPUT, Pin_Status::PULLUP);

            pin_write(_rst, 1);
            delay_ms(10);
            pin_write(_rst, 0);
            delay_ms(10);
            pin_write(_rst, 1);
            delay_ms(10);
        }

        if (Chip_Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        auto buffer = get_device_id();
        if (buffer != DEVICE_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get axp517 id fail (id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get axp517 id success (id: %#X)\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::end(void)
    {
        if (Chip_Iic_Guide::end() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "end fail\n");
            return false;
        }

        return true;
    }

    uint8_t Axp517::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Axp517::get_chip_status_0(Chip_Status_0 &status)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_BMU_STATUS0), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        status.current_limit_status = buffer & 0B00000001;
        status.thermal_regulation_status = (buffer & 0B00000010) >> 1;
        status.battery_in_active_mode = (buffer & 0B00000100) >> 2;
        status.battery_present_status = (buffer & 0B00001000) >> 3;
        status.batfet_status = (buffer & 0B00010000) >> 4;
        status.vbus_good_indication = (buffer & 0B00100000) >> 5;

        return true;
    }

    bool Axp517::get_chip_status_1(Chip_Status_1 &status)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_BMU_STATUS1), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        status.charging_status = [](uint8_t status) -> Charge_Status
        {
            switch (status)
            {
            case 0B00000000:
                return Charge_Status::TRICKLE_CHARGE;
            case 0B00000001:
                return Charge_Status::PRECHARGE;
            case 0B00000010:
                return Charge_Status::CONSTANT_CURRENT;
            case 0B00000011:
                return Charge_Status::CONSTANT_VOLTAGE;
            case 0B00000100:
                return Charge_Status::CHARGE_DONE;
            case 0B00000101:
                return Charge_Status::NOT_CHARGING;
            default:
                return Charge_Status::INVALID;
            } }(buffer & 0B00000111);

        status.vindpm_status = (buffer & 0B00001000) >> 3;
        status.system_status_indication = (buffer & 0B00010000) >> 4;

        status.battery_current_direction = [](uint8_t status) -> Battery_Current_Direction
        {
            switch (status)
            {
            case 0B00000000:
                return Battery_Current_Direction::STANDBY;
            case 0B00000001:
                return Battery_Current_Direction::CHARGE;
            case 0B00000010:
                return Battery_Current_Direction::DISCHARGE;
            default:
                return Battery_Current_Direction::INVALID;
            } }((buffer & 0B01100000) >> 5);

        return true;
    }

    // bool Axp517::get_irq_status(Irq_Status_0 &status0, Irq_Status_1 &status1, Irq_Status_2 &status2, Irq_Status_3 &status3)
    // {
    //     uint8_t buffer0 = 0, buffer1 = 0, buffer2 = 0, buffer3 = 0;

    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_IRQ_STATUS0), &buffer0) == false ||
    //         _bus->read(static_cast<uint8_t>(Cmd::RW_IRQ_STATUS1), &buffer1) == false ||
    //         _bus->read(static_cast<uint8_t>(Cmd::RW_IRQ_STATUS2), &buffer2) == false ||
    //         _bus->read(static_cast<uint8_t>(Cmd::RW_IRQ_STATUS3), &buffer3) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     // 解析状态0
    //     status0.vbus_fault_flag = (buffer0 & 0B00000001) >> 0;
    //     status0.vbus_over_voltage_flag = (buffer0 & 0B00000010) >> 1;
    //     status0.boost_over_voltage_flag = (buffer0 & 0B00000100) >> 2;
    //     status0.charge_to_normal_flag = (buffer0 & 0B00001000) >> 3;
    //     status0.gauge_new_soc_flag = (buffer0 & 0B00010000) >> 4;
    //     status0.soc_drop_to_shutdown_level_flag = (buffer0 & 0B01000000) >> 6;
    //     status0.soc_drop_to_warning_level_flag = (buffer0 & 0B10000000) >> 7;

    //     // 解析状态1
    //     status1.vbus_insert_flag = (buffer1 & 0B10000000) >> 7;
    //     status1.vbus_remove_flag = (buffer1 & 0B01000000) >> 6;
    //     status1.battery_insert_flag = (buffer1 & 0B00100000) >> 5;
    //     status1.battery_remove_flag = (buffer1 & 0B00010000) >> 4;
    //     status1.pwr_on_short_press_flag = (buffer1 & 0B00001000) >> 3;
    //     status1.pwr_on_long_press_flag = (buffer1 & 0B00000100) >> 2;
    //     status1.pwr_on_negative_edge_flag = (buffer1 & 0B00000010) >> 1;
    //     status1.pwr_on_positive_edge_flag = (buffer1 & 0B00000001) >> 0;

    //     // 解析状态2
    //     status2.watchdog_expire_flag = (buffer2 & 0B10000000) >> 7;
    //     status2.batfet_over_current_flag = (buffer2 & 0B00100000) >> 5;
    //     status2.battery_charge_done_flag = (buffer2 & 0B00010000) >> 4;
    //     status2.charger_start_flag = (buffer2 & 0B00001000) >> 3;
    //     status2.die_over_temperature_level1_flag = (buffer2 & 0B00000100) >> 2;
    //     status2.charger_safety_timer_expire_flag = (buffer2 & 0B00000010) >> 1;
    //     status2.battery_over_voltage_flag = (buffer2 & 0B00000001) >> 0;

    //     // 解析状态3
    //     status3.bc1_2_detect_finished_flag = (buffer3 & 0B10000000) >> 7;
    //     status3.bc1_2_detect_result_change_flag = (buffer3 & 0B01000000) >> 6;
    //     status3.battery_over_temperature_quit_flag = (buffer3 & 0B00010000) >> 4;
    //     status3.battery_over_temperature_charge_flag = (buffer3 & 0B00001000) >> 3;
    //     status3.battery_under_temperature_charge_flag = (buffer3 & 0B00000100) >> 2;
    //     status3.battery_over_temperature_work_flag = (buffer3 & 0B00000010) >> 1;
    //     status3.battery_under_temperature_work_flag = (buffer3 & 0B00000001) >> 0;

    //     return true;
    // }

    // bool Axp517::clear_all_irq(void)
    // {
    //     // 通过写入1清除所有中断标志
    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_IRQ_STATUS0), static_cast<uint8_t>(0xFF)) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }
    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_IRQ_STATUS1), static_cast<uint8_t>(0xFF)) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }
    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_IRQ_STATUS2), static_cast<uint8_t>(0xFF)) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }
    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_IRQ_STATUS3), static_cast<uint8_t>(0xFF)) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    bool Axp517::set_charge_enable(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_MODULE_ENABLE_CONTROL1), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (enable == true)
        {
            buffer |= 0B00000010; // 设置bit1为1
        }
        else
        {
            buffer &= 0B11111101; // 清除bit1
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_MODULE_ENABLE_CONTROL1), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::set_charge_current(uint16_t current_ma)
    {
        // 范围检查：0-5120mA，64mA/步进
        if (current_ma > 5120)
        {
            current_ma = 5120;
        }

        uint8_t buffer = current_ma / 64;

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_ICC_SETTING), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::set_charge_voltage(uint16_t voltage_mv)
    {
        uint8_t reg_value = 0;

        if (voltage_mv < 3700) // 小于3.7V，选择3.6V
        {
            reg_value = 6; // 3.6V
        }
        else if (voltage_mv < 3900) // 3.7V-3.9V，选择3.8V
        {
            reg_value = 5; // 3.8V
        }
        else if (voltage_mv < 4050) // 3.9V-4.05V，选择4.0V
        {
            reg_value = 0; // 4.0V
        }
        else if (voltage_mv < 4150) // 4.05V-4.15V，选择4.1V
        {
            reg_value = 1; // 4.1V
        }
        else if (voltage_mv < 4275) // 4.15V-4.275V，选择4.2V
        {
            reg_value = 2; // 4.2V
        }
        else if (voltage_mv < 4375) // 4.275V-4.375V，选择4.35V
        {
            reg_value = 3; // 4.35V
        }
        else if (voltage_mv < 4700) // 4.375V-4.7V，选择4.4V
        {
            reg_value = 4; // 4.4V
        }
        else // 大于等于4.7V，选择5.0V
        {
            reg_value = 7; // 5.0V
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CV_CHARGER_VOLTAGE_SETTING), reg_value) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::set_input_current_limit(uint16_t limit_ma)
    {
        // 范围检查：100-3250mA，50mA/步进
        if (limit_ma < 100)
        {
            limit_ma = 100;
        }
        else if (limit_ma > 3250)
        {
            limit_ma = 3250;
        }

        uint8_t buffer = ((limit_ma - 100) / 50) << 2;

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_INPUT_CURRENT_LIMIT_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::set_input_voltage_limit(uint16_t limit_mv)
    {
        // 范围检查：3600-16200mV，100mV/步进
        if (limit_mv < 3600)
        {
            limit_mv = 3600;
        }
        else if (limit_mv > 16200)
        {
            limit_mv = 16200;
        }

        uint8_t buffer = (limit_mv - 3600) / 100 + 1;

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_INPUT_VOLTAGE_LIMIT_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint8_t Axp517::get_battery_level(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_BATTERY_PERCENTAGE), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        return static_cast<int8_t>(buffer);
    }

    uint8_t Axp517::get_battery_health(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_BATTERY_SOH), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        return static_cast<int8_t>(buffer);
    }

    int8_t Axp517::get_battery_temperature_celsius(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_BATTERY_TEMPERATURE), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        return static_cast<int8_t>(buffer);
    }

    bool Axp517::set_adc_channel(Adc_Channel channel)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_CHANNEL_ENABLE_CONTROL),
                        static_cast<uint8_t>(
                            channel.vbus_current_measure << 7 |
                            channel.battery_discharge_current_measure << 6 |
                            channel.battery_charge_current_measure << 5 |
                            channel.chip_temperature_measure << 4 |
                            channel.system_voltage_measure << 3 |
                            channel.vbus_voltage_measure << 2 |
                            channel.ts_value_measure << 1 |
                            channel.battery_voltage_measure)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint16_t Axp517::get_battery_voltage(void)
    {
        uint8_t vbat_h = 0, vbat_l = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_VBAT_H), &vbat_h) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_VBAT_L), &vbat_l) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(vbat_h & 0B00111111) << 8) | vbat_l;
    }

    float Axp517::get_battery_current(void)
    {
        uint8_t ibat_h = 0, ibat_l = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_IBAT_H), &ibat_h) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_IBAT_L), &ibat_l) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return static_cast<float>(static_cast<int16_t>((static_cast<uint16_t>(ibat_h) << 8) | ibat_l)) / 4.0;
    }

    float Axp517::get_ts_voltage(void)
    {
        uint8_t ts_h = 0, ts_l = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_TS_H), &ts_h) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_TS_L), &ts_l) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return static_cast<float>((static_cast<uint16_t>(ts_h & 0B00111111) << 8) | ts_l) / 2.0;
    }

    uint16_t Axp517::get_vbus_current(void)
    {
        uint8_t vbus_h = 0, vbus_l = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_VBUS_CURRENT_H), &vbus_h) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_VBUS_CURRENT_L), &vbus_l) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(vbus_h & 0B00111111) << 8) | vbus_l;
    }

    uint16_t Axp517::get_vbus_voltage(void)
    {
        uint8_t vbus_h = 0, vbus_l = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_VBUS_VOLTAGE_H), &vbus_h) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_VBUS_VOLTAGE_L), &vbus_l) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return ((static_cast<uint16_t>(vbus_h & 0B00111111) << 8) | vbus_l) * 2;
    }

    bool Axp517::set_adc_data_select(Adc_Data data_select)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_DATA_SELECT), static_cast<uint8_t>(data_select)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint16_t Axp517::get_adc_data(void)
    {
        uint8_t buffer_h = 0, buffer_l = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_ADC_DATA_H), &buffer_h) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_ADC_DATA_L), &buffer_l) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer_h & 0B00111111) << 8) | buffer_l;
    }

    float Axp517::get_chip_die_junction_temperature_celsius(void)
    {
        uint16_t buffer = get_adc_data();

        if (buffer == -1)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_adc_data fail\n");
            return -1;
        }

        return static_cast<float>(3552 - buffer) / 1.79 + 25.0;
    }

    uint16_t Axp517::get_system_voltage(void)
    {
        uint16_t buffer = get_adc_data();

        if (buffer == -1)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_adc_data fail\n");
            return -1;
        }

        return buffer;
    }

    float Axp517::get_charging_current(void)
    {
        uint16_t buffer = get_adc_data();

        if (buffer == -1)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_adc_data fail\n");
            return -1;
        }

        return static_cast<float>(buffer) / 4.0;
    }

    float Axp517::get_discharging_current(void)
    {
        uint16_t buffer = get_adc_data();

        if (buffer == -1)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_adc_data fail\n");
            return -1;
        }

        return static_cast<float>(buffer) / 4.0;
    }

    bool Axp517::set_boost_enable(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_MODULE_ENABLE_CONTROL1), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (enable == true)
        {
            buffer |= 0B00010000;
        }
        else
        {
            buffer &= 0B11101111;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_MODULE_ENABLE_CONTROL1), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::set_gpio_source(Gpio_Source source)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_GPIO_CONFIGURE), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B11110011) | (static_cast<uint8_t>(source) << 2);

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_GPIO_CONFIGURE), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::set_gpio_mode(Gpio_Mode mode)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_GPIO_CONFIGURE), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (mode == Gpio_Mode::INPUT)
        {
            buffer &= 0B11101111;
        }
        else
        {
            buffer |= 0B00010000;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_GPIO_CONFIGURE), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::gpio_write(Gpio_Status status)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_GPIO_CONFIGURE), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B11111100) | static_cast<uint8_t>(status);

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_GPIO_CONFIGURE), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    Axp517::Gpio_Status Axp517::gpio_read(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_GPIO_CONFIGURE), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return Gpio_Status::INVALID;
        }

        if (((buffer & 0B00000010) >> 1) == 1)
        {
            return Gpio_Status::HIGH;
        }

        return Gpio_Status::LOW;
    }

    bool Axp517::set_shipping_mode_enable(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_BATFET_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (enable == true)
        {
            buffer |= 0B00001000;
        }
        else
        {
            buffer &= 0B11110111;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_BATFET_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::set_force_batfet_mode(Force_Batfet mode)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_BATFET_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        switch (mode)
        {
        case Force_Batfet::AUTO:
            buffer &= 0B11111010;
            break;
        case Force_Batfet::ON:
            buffer &= 0B11111011;
            buffer |= 0B00000001;
            break;
        case Force_Batfet::OFF:
            buffer &= 0B11111110;
            buffer |= 0B00000100;
            break;

        default:
            break;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_BATFET_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Axp517::set_force_rbfet_enable(bool enable)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_RBFET_CONTROL), static_cast<uint8_t>(enable)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    // bool Axp517::set_boost_voltage(uint16_t voltage_mv)
    // {
    //     if (voltage_mv < 4550)
    //     {
    //         voltage_mv = 4550;
    //     }
    //     else if (voltage_mv > 5510)
    //     {
    //         voltage_mv = 5510;
    //     }

    //     uint8_t n = (voltage_mv - 4550) / 64;
    //     if (n > 15)
    //     {
    //         n = 15;
    //     }

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_BOOST_CONFIGURE), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer = (buffer & 0x0F) | (n << 4);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_BOOST_CONFIGURE), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Axp517::set_watchdog(bool enable, uint8_t timeout_s)
    // {
    //     uint8_t buffer = 0;

    //     // 设置看门狗使能
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_MODULE_ENABLE_CONTROL1), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable == true)
    //     {
    //         buffer |= 0B00000001; // 设置bit0为1
    //     }
    //     else
    //     {
    //         buffer &= 0B11111110; // 清除bit0
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_MODULE_ENABLE_CONTROL1), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     // 设置看门狗超时时间
    //     uint8_t timeout_value = 0;
    //     if (timeout_s <= 1)
    //     {
    //         timeout_value = 0; // 1s
    //     }
    //     else if (timeout_s <= 2)
    //     {
    //         timeout_value = 1; // 2s
    //     }
    //     else if (timeout_s <= 4)
    //     {
    //         timeout_value = 2; // 4s
    //     }
    //     else if (timeout_s <= 8)
    //     {
    //         timeout_value = 3; // 8s
    //     }
    //     else if (timeout_s <= 16)
    //     {
    //         timeout_value = 4; // 16s
    //     }
    //     else if (timeout_s <= 32)
    //     {
    //         timeout_value = 5; // 32s
    //     }
    //     else if (timeout_s <= 64)
    //     {
    //         timeout_value = 6; // 64s
    //     }
    //     else
    //     {
    //         timeout_value = 7; // 128s
    //     }

    //     buffer = (timeout_value & 0x07);
    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_WATCHDOG_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Axp517::feed_watchdog(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_WATCHDOG_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer |= 0B00001000; // 设置bit3为1，清除看门狗

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_WATCHDOG_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Axp517::set_jeita_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_JEITA_STANDARD_ENABLE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable == true)
    //     {
    //         buffer |= 0B00000001; // 设置bit0为1
    //     }
    //     else
    //     {
    //         buffer &= 0B11111110; // 清除bit0
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_JEITA_STANDARD_ENABLE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Axp517::set_bc12_detect_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_MODULE_ENABLE_CONTROL0), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable == true)
    //     {
    //         buffer |= 0B00010000; // 设置bit4为1
    //     }
    //     else
    //     {
    //         buffer &= 0B11101111; // 清除bit4
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_MODULE_ENABLE_CONTROL0), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Axp517::get_bc12_detect_result(Bc_Detect_Result &result)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RO_BC_DETECT), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     uint8_t detect_result = (buffer & 0B11100000) >> 5;

    //     switch (detect_result)
    //     {
    //     case 1:
    //         result = Bc_Detect_Result::SDP;
    //         break;
    //     case 2:
    //         result = Bc_Detect_Result::CDP;
    //         break;
    //     case 3:
    //         result = Bc_Detect_Result::DCP;
    //         break;
    //     default:
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid BC detect result\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Axp517::set_pd_role(bool is_source, bool is_drp)
    // {
    //     uint8_t buffer = 0;

    //     // 设置角色控制
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_ROLE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     // 设置DRP
    //     if (is_drp == true)
    //     {
    //         buffer |= 0B01000000; // 设置bit6为1
    //     }
    //     else
    //     {
    //         buffer &= 0B10111111; // 清除bit6
    //     }

    //     // 设置CC1和CC2为相同角色
    //     if (is_source == true)
    //     {
    //         buffer = (buffer & 0B11001111) | 0B00010000; // CC1和CC2为Rp
    //     }
    //     else
    //     {
    //         buffer = (buffer & 0B11001111) | 0B00100000; // CC1和CC2为Rd
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_ROLE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     // 如果需要DRP模式，发送Look4Connection命令
    //     if (is_drp == true)
    //     {
    //         if (_bus->write(static_cast<uint8_t>(Cmd::RW_COMMAND), static_cast<uint8_t>(0x99)) == false)
    //         {
    //             assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //             return false;
    //         }
    //     }

    //     return true;
    // }
}