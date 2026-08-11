/*
 * @Description: ESP-IDF LEDC PWM 驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-08-10 18:07:50
 * @LastEditTime: 2026-08-11 09:09:46
 * @License: GPL 3.0
 */
#include "pwm.h"

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)

#include <array>
#include <cstddef>
#include <limits>

#include "soc/soc_caps.h"

namespace cpp_bus_driver {
namespace {

static_assert(SOC_LEDC_TIMER_BIT_WIDTH < 32,
    "Pwm duty conversion requires an LEDC counter narrower than 32 bits");

struct TimerUsage {
  uint32_t reference_count = 0;
  uint32_t frequency_hz = 0;
  ledc_timer_bit_t resolution = LEDC_TIMER_1_BIT;
  ledc_clk_cfg_t clock_source = LEDC_AUTO_CLK;
};

struct LedcResources {
  std::mutex mutex;
  std::array<std::array<bool, LEDC_CHANNEL_MAX>, LEDC_SPEED_MODE_MAX>
      channels_in_use{};
  std::array<std::array<TimerUsage, LEDC_TIMER_MAX>, LEDC_SPEED_MODE_MAX>
      timers{};
  std::array<bool, GPIO_NUM_MAX> pins_in_use{};
  bool fade_service_installed = false;
};

/**
 * @brief 获取进程生命周期内唯一的 LEDC 资源管理器
 * @return LEDC 资源管理器引用
 */
LedcResources& GetLedcResources() {
  // 资源管理器与 LEDC 全局驱动同生命周期，避免静态析构顺序影响 Pwm 析构。
  static LedcResources* const resources = new LedcResources();
  return *resources;
}

/**
 * @brief 将 LEDC 速度模式转换为数组索引
 * @param value LEDC 速度模式
 * @return 对应的数组索引
 */
std::size_t ToIndex(ledc_mode_t value) {
  return static_cast<std::size_t>(value);
}

/**
 * @brief 将 LEDC 通道转换为数组索引
 * @param value LEDC 通道
 * @return 对应的数组索引
 */
std::size_t ToIndex(ledc_channel_t value) {
  return static_cast<std::size_t>(value);
}

/**
 * @brief 将 LEDC 定时器转换为数组索引
 * @param value LEDC 定时器
 * @return 对应的数组索引
 */
std::size_t ToIndex(ledc_timer_t value) {
  return static_cast<std::size_t>(value);
}

/**
 * @brief 检查 PWM 配置中的 LEDC 资源索引是否有效
 * @param config 待检查的 PWM 配置
 * @return 所有资源索引有效返回 true，否则返回 false
 */
bool IsConfigIndexValid(const Pwm::Config& config) {
  const int speed_mode = static_cast<int>(config.speed_mode);
  const int timer = static_cast<int>(config.timer);
  const int channel = static_cast<int>(config.channel);
  return speed_mode >= 0 && speed_mode < LEDC_SPEED_MODE_MAX && timer >= 0 &&
         timer < LEDC_TIMER_MAX && channel >= 0 && channel < LEDC_CHANNEL_MAX;
}

/**
 * @brief 检查共享定时器配置是否一致
 * @param usage 当前记录的定时器配置
 * @param config 待使用的 PWM 配置
 * @return 配置一致返回 true，否则返回 false
 */
bool TimerConfigMatches(const TimerUsage& usage, const Pwm::Config& config) {
  return usage.frequency_hz == config.frequency_hz &&
         usage.resolution == config.resolution &&
         usage.clock_source == config.clock_source;
}

/**
 * @brief 确保 LEDC 全局渐变服务已经安装
 * @param resources LEDC 资源管理器
 * @return 安装成功或已经安装返回 ESP_OK，否则返回对应错误码
 */
esp_err_t EnsureFadeServiceInstalled(LedcResources* resources) {
  if (resources->fade_service_installed) {
    return ESP_OK;
  }

  const esp_err_t result = ledc_fade_func_install(0);
  if (result == ESP_OK || result == ESP_ERR_INVALID_STATE) {
    resources->fade_service_installed = true;
    return ESP_OK;
  }
  return result;
}

}  // namespace

bool Pwm::Init(const Config& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (initialized_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Pwm has already been initialized (pin: %ld)\n",
        static_cast<long>(pin_));
    return false;
  }

  uint32_t initial_raw_duty = 0;
  if (!ValidateConfig(config, &initial_raw_duty)) {
    return false;
  }

  LedcResources& resources = GetLedcResources();
  std::lock_guard<std::mutex> resource_lock(resources.mutex);
  const std::size_t speed_index = ToIndex(config.speed_mode);
  const std::size_t channel_index = ToIndex(config.channel);
  const std::size_t timer_index = ToIndex(config.timer);
  const std::size_t pin_index = static_cast<std::size_t>(pin_);
  if (resources.channels_in_use[speed_index][channel_index]) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm channel is already in use (speed mode: %u, channel: %u)\n",
        static_cast<unsigned int>(speed_index),
        static_cast<unsigned int>(channel_index));
    return false;
  }
  if (resources.pins_in_use[pin_index]) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm output pin is already in use (pin: %ld)\n",
        static_cast<long>(pin_));
    return false;
  }

  TimerUsage& timer_usage = resources.timers[speed_index][timer_index];
  if (timer_usage.reference_count != 0 &&
      !TimerConfigMatches(timer_usage, config)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm timer configuration conflicts with an active channel "
        "(speed mode: %u, timer: %u)\n",
        static_cast<unsigned int>(speed_index),
        static_cast<unsigned int>(timer_index));
    return false;
  }

  const bool configure_timer = timer_usage.reference_count == 0;
  if (configure_timer) {
    const ledc_timer_config_t timer_config = {
        .speed_mode = config.speed_mode,
        .duty_resolution = config.resolution,
        .timer_num = config.timer,
        .freq_hz = config.frequency_hz,
        .clk_cfg = config.clock_source,
        .deconfigure = false,
    };
    const esp_err_t result = ledc_timer_config(&timer_config);
    if (result != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "ledc_timer_config failed (error code: %#X)\n",
          static_cast<unsigned int>(result));
      return false;
    }
  }

  const ledc_channel_config_t channel_config = {
      .gpio_num = pin_,
      .speed_mode = config.speed_mode,
      .channel = config.channel,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = config.timer,
      .duty = initial_raw_duty,
      .hpoint = static_cast<int>(config.hpoint),
      // ESP-IDF 5.5.4 的 legacy LEDC 无法撤销通道的睡眠保持状态，因此仅使用
      // 可完整结束生命周期的默认模式。
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
      .flags =
          {
              .output_invert = config.output_inverted,
          },
  };
  esp_err_t result = ledc_channel_config(&channel_config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_channel_config failed (error code: %#X)\n",
        static_cast<unsigned int>(result));
    if (configure_timer) {
      static_cast<void>(DeconfigureTimerLocked(config));
    }
    return false;
  }

  result = EnsureFadeServiceInstalled(&resources);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_fade_func_install failed (error code: %#X)\n",
        static_cast<unsigned int>(result));
    RollbackInitLocked(config, configure_timer);
    return false;
  }

  // 提前创建 ESP-IDF 的通道同步资源，避免首次调节占空比时动态分配。
  result = ledc_set_duty_and_update(
      config.speed_mode, config.channel, initial_raw_duty, config.hpoint);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_set_duty_and_update failed during Pwm initialization "
        "(error code: %#X)\n",
        static_cast<unsigned int>(result));
    RollbackInitLocked(config, configure_timer);
    return false;
  }

  resources.channels_in_use[speed_index][channel_index] = true;
  resources.pins_in_use[pin_index] = true;
  if (timer_usage.reference_count == 0) {
    timer_usage.frequency_hz = config.frequency_hz;
    timer_usage.resolution = config.resolution;
    timer_usage.clock_source = config.clock_source;
  }
  ++timer_usage.reference_count;

  config_ = config;
  initialized_ = true;
  non_blocking_fade_may_be_active_ = false;
  return true;
}

bool Pwm::Deinit() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_) {
    return true;
  }

  LedcResources& resources = GetLedcResources();
  std::lock_guard<std::mutex> resource_lock(resources.mutex);
  bool result = CancelFadeLocked();
  const esp_err_t stop_result = ledc_stop(config_.speed_mode, config_.channel,
      ToDriverIdleLevel(config_.idle_level_on_deinit, config_.output_inverted));
  if (stop_result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_stop failed during Pwm deinitialization (error code: %#X)\n",
        static_cast<unsigned int>(stop_result));
    result = false;
  }
  if (!ReleaseOutputPin(config_.idle_level_on_deinit)) {
    result = false;
  }

  const std::size_t speed_index = ToIndex(config_.speed_mode);
  const std::size_t channel_index = ToIndex(config_.channel);
  const std::size_t timer_index = ToIndex(config_.timer);
  resources.channels_in_use[speed_index][channel_index] = false;
  resources.pins_in_use[static_cast<std::size_t>(pin_)] = false;
  TimerUsage& timer_usage = resources.timers[speed_index][timer_index];
  if (timer_usage.reference_count > 0) {
    --timer_usage.reference_count;
    if (timer_usage.reference_count == 0) {
      if (!DeconfigureTimerLocked(config_)) {
        result = false;
      }
      timer_usage = TimerUsage();
    }
  }

  initialized_ = false;
  non_blocking_fade_may_be_active_ = false;
  return result;
}

bool Pwm::IsInitialized() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return initialized_;
}

bool Pwm::SetDuty(DutyCycle duty) {
  std::lock_guard<std::mutex> lock(mutex_);
  return SetDutyLocked(duty);
}

bool Pwm::FadeTo(DutyCycle target_duty, uint32_t duration_ms, FadeMode mode) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm is not initialized (pin: %ld)\n", static_cast<long>(pin_));
    return false;
  }
  if (mode != FadeMode::kWaitForCompletion && mode != FadeMode::kNoWait) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm fade mode (value: %u)\n", static_cast<unsigned int>(mode));
    return false;
  }
  if (duration_ms == 0) {
    return SetDutyLocked(target_duty);
  }

#if !SOC_LEDC_SUPPORT_FADE_STOP
  if (mode == FadeMode::kNoWait) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Non-blocking Pwm fade is not supported by this target\n");
    return false;
  }
#endif
  if (duration_ms > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm fade duration exceeds the ESP-IDF limit (duration: %u ms)\n",
        static_cast<unsigned int>(duration_ms));
    return false;
  }
  uint32_t target_raw_duty = 0;
  if (!ConvertDutyCycleToRaw(
          target_duty, config_.resolution, &target_raw_duty)) {
    return false;
  }

  const uint32_t actual_frequency_hz =
      ledc_get_freq(config_.speed_mode, config_.timer);
  if (actual_frequency_hz == 0) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_get_freq failed during Pwm fade\n");
    return false;
  }
  if (duration_ms >
      std::numeric_limits<uint32_t>::max() / actual_frequency_hz) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm fade duration is too long (duration: %u ms, frequency: %u Hz)\n",
        static_cast<unsigned int>(duration_ms),
        static_cast<unsigned int>(actual_frequency_hz));
    return false;
  }
  const uint64_t fade_cycle_numerator =
      static_cast<uint64_t>(duration_ms) * actual_frequency_hz;
  if (fade_cycle_numerator < 1000) {
    return SetDutyLocked(target_duty);
  }
  if (!CancelFadeLocked()) {
    return false;
  }

  const ledc_fade_mode_t driver_mode = mode == FadeMode::kWaitForCompletion
                                           ? LEDC_FADE_WAIT_DONE
                                           : LEDC_FADE_NO_WAIT;
  const esp_err_t result = ledc_set_fade_time_and_start(config_.speed_mode,
      config_.channel, target_raw_duty, duration_ms, driver_mode);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_set_fade_time_and_start failed (error code: %#X)\n",
        static_cast<unsigned int>(result));
    return false;
  }

  non_blocking_fade_may_be_active_ = mode == FadeMode::kNoWait;
  return true;
}

bool Pwm::CancelFade() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm is not initialized (pin: %ld)\n", static_cast<long>(pin_));
    return false;
  }
  return CancelFadeLocked();
}

bool Pwm::DisableOutput(IdleLevel idle_level) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm is not initialized (pin: %ld)\n", static_cast<long>(pin_));
    return false;
  }
  if (!IsIdleLevelValid(idle_level)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm idle level (value: %u)\n",
        static_cast<unsigned int>(idle_level));
    return false;
  }
  if (!CancelFadeLocked()) {
    return false;
  }

  const esp_err_t result = ledc_stop(config_.speed_mode, config_.channel,
      ToDriverIdleLevel(idle_level, config_.output_inverted));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_stop failed (error code: %#X)\n",
        static_cast<unsigned int>(result));
    return false;
  }
  return true;
}

bool Pwm::SetTimerFrequencyHz(uint32_t frequency_hz) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm is not initialized (pin: %ld)\n", static_cast<long>(pin_));
    return false;
  }
  if (frequency_hz == 0 ||
      frequency_hz > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm frequency (frequency: %u Hz)\n",
        static_cast<unsigned int>(frequency_hz));
    return false;
  }

  LedcResources& resources = GetLedcResources();
  std::lock_guard<std::mutex> resource_lock(resources.mutex);
  TimerUsage& timer_usage =
      resources.timers[ToIndex(config_.speed_mode)][ToIndex(config_.timer)];
  if (timer_usage.reference_count != 1) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm timer frequency cannot change while the timer is shared "
        "(references: %u)\n",
        static_cast<unsigned int>(timer_usage.reference_count));
    return false;
  }
  if (!CancelFadeLocked()) {
    return false;
  }

  const esp_err_t result =
      ledc_set_freq(config_.speed_mode, config_.timer, frequency_hz);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_set_freq failed (error code: %#X)\n",
        static_cast<unsigned int>(result));
    return false;
  }

  config_.frequency_hz = frequency_hz;
  timer_usage.frequency_hz = frequency_hz;
  return true;
}

bool Pwm::CancelFadeLocked() {
  if (!non_blocking_fade_may_be_active_) {
    return true;
  }

#if SOC_LEDC_SUPPORT_FADE_STOP
  const esp_err_t result = ledc_fade_stop(config_.speed_mode, config_.channel);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_fade_stop failed (error code: %#X)\n",
        static_cast<unsigned int>(result));
    return false;
  }
  non_blocking_fade_may_be_active_ = false;
  return true;
#else
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "Pwm fade cancellation is not supported by this target\n");
  return false;
#endif
}

bool Pwm::ConvertDutyCycleToRaw(
    DutyCycle duty, ledc_timer_bit_t resolution, uint32_t* raw_duty) {
  if (raw_duty == nullptr || duty.scale == 0 || duty.value > duty.scale) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm duty cycle (value: %u, scale: %u)\n",
        static_cast<unsigned int>(duty.value),
        static_cast<unsigned int>(duty.scale));
    return false;
  }

  const int resolution_bits = static_cast<int>(resolution);
  if (resolution_bits <= 0 || resolution_bits > SOC_LEDC_TIMER_BIT_WIDTH ||
      resolution_bits >= 32) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm duty resolution (bits: %d)\n", resolution_bits);
    return false;
  }

  const uint64_t period = uint64_t{1} << resolution_bits;
  // ESP32-P4 ECO2 等芯片在硬件最大分辨率下不能写入 2^resolution。
  const uint64_t maximum_raw_duty =
      resolution_bits == SOC_LEDC_TIMER_BIT_WIDTH ? period - 1 : period;
  const uint64_t scaled_duty =
      static_cast<uint64_t>(duty.value) * maximum_raw_duty;
  *raw_duty =
      static_cast<uint32_t>((scaled_duty + duty.scale / 2U) / duty.scale);
  return true;
}

bool Pwm::DeconfigureTimerLocked(const Config& config) {
  esp_err_t result = ledc_timer_pause(config.speed_mode, config.timer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_timer_pause failed (error code: %#X)\n",
        static_cast<unsigned int>(result));
    return false;
  }

  const ledc_timer_config_t timer_config = {
      .speed_mode = config.speed_mode,
      .duty_resolution = config.resolution,
      .timer_num = config.timer,
      .freq_hz = config.frequency_hz,
      .clk_cfg = config.clock_source,
      .deconfigure = true,
  };
  result = ledc_timer_config(&timer_config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_timer_config failed while releasing Pwm timer "
        "(error code: %#X)\n",
        static_cast<unsigned int>(result));
    return false;
  }
  return true;
}

bool Pwm::ReleaseOutputPin(IdleLevel idle_level) {
  bool result = true;
  const gpio_num_t gpio = static_cast<gpio_num_t>(pin_);
  esp_err_t operation_result = gpio_reset_pin(gpio);
  if (operation_result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_reset_pin failed while releasing Pwm output "
        "(pin: %ld, error code: %#X)\n",
        static_cast<long>(pin_), static_cast<unsigned int>(operation_result));
    result = false;
  }

  // 先写输出锁存值再切换方向，尽量缩短 LEDC 释放后的电平过渡时间。
  const uint32_t physical_level = idle_level == IdleLevel::kHigh ? 1U : 0U;
  operation_result = gpio_set_level(gpio, physical_level);
  if (operation_result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_set_level failed while releasing Pwm output "
        "(pin: %ld, error code: %#X)\n",
        static_cast<long>(pin_), static_cast<unsigned int>(operation_result));
    result = false;
  }
  operation_result = gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
  if (operation_result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_set_direction failed while releasing Pwm output "
        "(pin: %ld, error code: %#X)\n",
        static_cast<long>(pin_), static_cast<unsigned int>(operation_result));
    result = false;
  }
  operation_result = gpio_pullup_dis(gpio);
  if (operation_result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_pullup_dis failed while releasing Pwm output "
        "(pin: %ld, error code: %#X)\n",
        static_cast<long>(pin_), static_cast<unsigned int>(operation_result));
    result = false;
  }
  operation_result = gpio_pulldown_dis(gpio);
  if (operation_result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_pulldown_dis failed while releasing Pwm output "
        "(pin: %ld, error code: %#X)\n",
        static_cast<long>(pin_), static_cast<unsigned int>(operation_result));
    result = false;
  }
  return result;
}

void Pwm::RollbackInitLocked(const Config& config, bool deconfigure_timer) {
  static_cast<void>(ledc_stop(config.speed_mode, config.channel,
      ToDriverIdleLevel(config.idle_level_on_deinit, config.output_inverted)));
  static_cast<void>(ReleaseOutputPin(config.idle_level_on_deinit));
  if (deconfigure_timer) {
    static_cast<void>(DeconfigureTimerLocked(config));
  }
}

bool Pwm::SetDutyLocked(DutyCycle duty) {
  if (!initialized_) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Pwm is not initialized (pin: %ld)\n", static_cast<long>(pin_));
    return false;
  }
  uint32_t raw_duty = 0;
  if (!ConvertDutyCycleToRaw(duty, config_.resolution, &raw_duty)) {
    return false;
  }
  if (!CancelFadeLocked()) {
    return false;
  }
  const esp_err_t result = ledc_set_duty_and_update(
      config_.speed_mode, config_.channel, raw_duty, config_.hpoint);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_set_duty_and_update failed (error code: %#X)\n",
        static_cast<unsigned int>(result));
    return false;
  }
  return true;
}

bool Pwm::ValidateConfig(const Config& config, uint32_t* initial_raw_duty) {
  if (!GPIO_IS_VALID_OUTPUT_GPIO(pin_)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm output pin (pin: %ld)\n", static_cast<long>(pin_));
    return false;
  }
  if (!IsConfigIndexValid(config)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm LEDC resource (speed mode: %d, timer: %d, channel: %d)\n",
        static_cast<int>(config.speed_mode), static_cast<int>(config.timer),
        static_cast<int>(config.channel));
    return false;
  }
  if (config.frequency_hz == 0 ||
      config.frequency_hz >
          static_cast<uint32_t>(std::numeric_limits<int>::max())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm frequency (frequency: %u Hz)\n",
        static_cast<unsigned int>(config.frequency_hz));
    return false;
  }
  if (!IsIdleLevelValid(config.idle_level_on_deinit)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm idle level (value: %u)\n",
        static_cast<unsigned int>(config.idle_level_on_deinit));
    return false;
  }
  if (!ConvertDutyCycleToRaw(
          config.initial_duty, config.resolution, initial_raw_duty)) {
    return false;
  }

  const int resolution_bits = static_cast<int>(config.resolution);
  const uint64_t period = uint64_t{1} << resolution_bits;
  if (config.hpoint >= period) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid Pwm hpoint (hpoint: %u, maximum: %u)\n",
        static_cast<unsigned int>(config.hpoint),
        static_cast<unsigned int>(period - 1));
    return false;
  }
  return true;
}

uint32_t Pwm::ToDriverIdleLevel(IdleLevel idle_level, bool output_inverted) {
  const bool physical_high = idle_level == IdleLevel::kHigh;
  return physical_high ^ output_inverted ? 1U : 0U;
}

bool Pwm::IsIdleLevelValid(IdleLevel idle_level) {
  return idle_level == IdleLevel::kLow || idle_level == IdleLevel::kHigh;
}

}  // namespace cpp_bus_driver

#endif
