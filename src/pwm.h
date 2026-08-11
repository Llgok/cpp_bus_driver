/*
 * @Description: ESP-IDF LEDC PWM 驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-08-10 18:07:50
 * @LastEditTime: 2026-08-11 09:12:05
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>

#include "tool.h"

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
#include <mutex>
#endif

namespace cpp_bus_driver {

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)

class Pwm final : private Tool {
 public:
  enum class FadeMode : uint8_t {
    kWaitForCompletion,
    kNoWait,
  };

  // 表示 PWM 停止后物理引脚保持的电平。
  enum class IdleLevel : uint8_t {
    kLow,
    kHigh,
  };

  // value / scale 表示占空比；输入比例可使用任意精度，实际输出精度受硬件
  // 分辨率限制。
  struct DutyCycle {
    uint32_t value = 0;
    uint32_t scale = 1;
  };

  struct Config {
    ledc_timer_t timer = LEDC_TIMER_0;
    ledc_channel_t channel = LEDC_CHANNEL_0;
    uint32_t frequency_hz = 0;
    ledc_timer_bit_t resolution = LEDC_TIMER_10_BIT;
    ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_clk_cfg_t clock_source = LEDC_AUTO_CLK;
    DutyCycle initial_duty;
    uint32_t hpoint = 0;
    IdleLevel idle_level_on_deinit = IdleLevel::kLow;
    bool output_inverted = false;
  };

  explicit Pwm(int32_t pin) : pin_(pin) {}

  ~Pwm() override { Deinit(); }

  // PWM 对象独占硬件资源，不支持复制或移动。
  Pwm(const Pwm&) = delete;
  Pwm& operator=(const Pwm&) = delete;
  Pwm(Pwm&&) = delete;
  Pwm& operator=(Pwm&&) = delete;

  /**
   * @brief 初始化 PWM 定时器和输出通道
   * @param config PWM 配置；同一 speed mode 和 channel 不能被多个 Pwm 占用
   * @return 初始化成功返回 true，失败返回 false
   */
  bool Init(const Config& config);

  /**
   * @brief 停止输出并结束当前 PWM 生命周期
   * @return 所有硬件清理操作成功返回 true，否则返回 false
   * @note 此函数不会卸载 LEDC 全局渐变服务；最后一个通道退出时释放定时器
   *       和 LEDC GPIO 所有权，并使用普通 GPIO 保持配置的物理空闲电平
   * @note 即使硬件清理失败，软件生命周期仍会结束，后续可重新调用 Init
   */
  bool Deinit();

  /**
   * @brief 查询 PWM 是否已经完成初始化
   * @return 已初始化返回 true，否则返回 false
   */
  bool IsInitialized() const;

  /**
   * @brief 使用任意比例设置 PWM 占空比
   * @param duty 占空比，必须满足 scale > 0 且 value <= scale
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDuty(DutyCycle duty);

  /**
   * @brief 在指定时间内渐变到目标占空比
   * @param target_duty 目标占空比
   * @param duration_ms 期望渐变时间；硬件取整可能使实际时间存在偏差
   * @param mode 等待渐变完成或立即返回
   * @return 启动并按指定模式执行成功返回 true，失败返回 false
   * @note kWaitForCompletion 会阻塞当前任务，并串行等待同一对象的其他操作
   */
  bool FadeTo(DutyCycle target_duty, uint32_t duration_ms, FadeMode mode);

  /**
   * @brief 取消由非阻塞模式启动的渐变
   * @return 取消成功或当前没有待取消渐变时返回 true
   */
  bool CancelFade();

  /**
   * @brief 禁用 PWM 输出但保留已初始化配置
   * @param idle_level 禁用后物理引脚保持的电平
   * @return 禁用成功返回 true，失败返回 false
   * @note 后续调用 SetDuty 或 FadeTo 会自动恢复 PWM 输出
   */
  bool DisableOutput(IdleLevel idle_level);

  /**
   * @brief 修改当前 PWM 定时器频率
   * @param frequency_hz 新频率，单位为 Hz
   * @return 设置成功返回 true，失败返回 false
   * @note 定时器属于共享资源；存在其他 Pwm 通道引用时拒绝修改
   */
  bool SetTimerFrequencyHz(uint32_t frequency_hz);

 private:
  /**
   * @brief 在持有对象互斥锁时取消非阻塞渐变
   * @return 取消成功或当前没有待取消渐变时返回 true
   */
  bool CancelFadeLocked();

  /**
   * @brief 将比例占空比转换为 LEDC 硬件计数值
   * @param duty 比例占空比
   * @param resolution LEDC 占空比分辨率
   * @param raw_duty 转换后的硬件计数值
   * @return 转换成功返回 true，参数无效返回 false
   */
  bool ConvertDutyCycleToRaw(
      DutyCycle duty, ledc_timer_bit_t resolution, uint32_t* raw_duty);

  /**
   * @brief 在持有资源互斥锁时释放 LEDC 定时器
   * @param config 当前 PWM 配置
   * @return 释放成功返回 true，失败返回 false
   */
  bool DeconfigureTimerLocked(const Config& config);

  /**
   * @brief 释放 LEDC 引脚并将其恢复为普通 GPIO 输出
   * @param idle_level 释放后物理引脚保持的电平
   * @return 所有 GPIO 操作成功返回 true，否则返回 false
   */
  bool ReleaseOutputPin(IdleLevel idle_level);

  /**
   * @brief 在持有资源互斥锁时回滚未完成的初始化
   * @param config 本次初始化使用的 PWM 配置
   * @param deconfigure_timer 是否同时释放本次创建的定时器
   */
  void RollbackInitLocked(const Config& config, bool deconfigure_timer);

  /**
   * @brief 在持有对象互斥锁时设置 PWM 占空比
   * @param duty 目标占空比
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDutyLocked(DutyCycle duty);

  /**
   * @brief 校验 PWM 配置并转换初始占空比
   * @param config 待校验的 PWM 配置
   * @param initial_raw_duty 转换后的初始硬件占空比
   * @return 配置有效返回 true，否则返回 false
   */
  bool ValidateConfig(const Config& config, uint32_t* initial_raw_duty);

  /**
   * @brief 将物理空闲电平转换为 LEDC 驱动使用的逻辑电平
   * @param idle_level 物理空闲电平
   * @param output_inverted 是否反转 PWM 输出
   * @return LEDC 驱动使用的逻辑空闲电平
   */
  static uint32_t ToDriverIdleLevel(IdleLevel idle_level, bool output_inverted);

  /**
   * @brief 检查物理空闲电平是否有效
   * @param idle_level 待检查的物理空闲电平
   * @return 电平有效返回 true，否则返回 false
   */
  static bool IsIdleLevelValid(IdleLevel idle_level);

  const int32_t pin_;
  mutable std::mutex mutex_;
  Config config_;
  bool initialized_ = false;
  bool non_blocking_fade_may_be_active_ = false;
};
#endif

}  // namespace cpp_bus_driver
