/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 10:22:46
 * @LastEditTime: 2026-07-13 22:36:55
 * @License: GPL 3.0
 */
#include "tool.h"

#include <cerrno>
#include <cctype>
#include <cstdlib>

namespace cpp_bus_driver {
namespace {

/**
 * @brief 获取日志等级名称
 * @param level 日志等级
 * @return 日志等级名称
 */
const char* LogLevelName(Tool::LogLevel level) {
  switch (level) {
    case Tool::LogLevel::kDebug:
      return "Debug";
    case Tool::LogLevel::kInfo:
      return "Info";
    case Tool::LogLevel::kWarning:
      return "Warning";
    case Tool::LogLevel::kError:
      return "Error";
    default:
      return "Unknown";
  }
}

/**
 * @brief 判断指定日志等级是否允许输出
 * @param level 日志等级
 * @return 允许输出返回 true，否则返回 false
 */
bool IsLogLevelEnabled(Tool::LogLevel level) {
  switch (level) {
#if defined(CPP_BUS_DRIVER_LOG_LEVEL_DEBUG)
    case Tool::LogLevel::kDebug:
      return true;
#endif
#if defined(CPP_BUS_DRIVER_LOG_LEVEL_INFO)
    case Tool::LogLevel::kInfo:
      return true;
#endif
#if defined(CPP_BUS_DRIVER_LOG_LEVEL_WARNING)
    case Tool::LogLevel::kWarning:
      return true;
#endif
#if defined(CPP_BUS_DRIVER_LOG_LEVEL_ERROR)
    case Tool::LogLevel::kError:
      return true;
#endif
    default:
      return false;
  }
}

/**
 * @brief 检查转换后剩余字符是否只包含空白字符
 * @param value 需要检查的字符串指针
 * @return 只剩空白字符或字符串结束返回 true
 */
bool HasOnlyTrailingSpace(const char* value) {
  if (value == nullptr) {
    return false;
  }

  while (*value != '\0') {
    if (!std::isspace(static_cast<unsigned char>(*value))) {
      return false;
    }
    value++;
  }

  return true;
}

}  // namespace

namespace safe_convert {

bool SafeStringToLong(const std::string& input, long* output) {
  if (input.empty() || output == nullptr) {
    return false;
  }

  errno = 0;
  char* endptr = nullptr;
  const char* begin = input.c_str();
  const long value = std::strtol(begin, &endptr, 10);
  if (endptr == begin || errno == ERANGE || !HasOnlyTrailingSpace(endptr)) {
    return false;
  }

  *output = value;
  return true;
}

bool SafeStringToFloat(const std::string& input, float* output) {
  if (input.empty() || output == nullptr) {
    return false;
  }

  errno = 0;
  char* endptr = nullptr;
  const char* begin = input.c_str();
  const float value = std::strtof(begin, &endptr);
  if (endptr == begin || errno == ERANGE || !std::isfinite(value) ||
      !HasOnlyTrailingSpace(endptr)) {
    return false;
  }

  *output = value;
  return true;
}

bool SafeStringToDouble(const std::string& input, double* output) {
  if (input.empty() || output == nullptr) {
    return false;
  }

  errno = 0;
  char* endptr = nullptr;
  const char* begin = input.c_str();
  const double value = std::strtod(begin, &endptr);
  if (endptr == begin || errno == ERANGE || !std::isfinite(value) ||
      !HasOnlyTrailingSpace(endptr)) {
    return false;
  }

  *output = value;
  return true;
}

}  // namespace safe_convert

void Tool::LogMessage(LogLevel level, const char* file_name, size_t line_number,
    const char* format, ...) {
  if (!IsLogLevelEnabled(level)) {
    return;
  }

  va_list args;
  va_start(args, format);
  auto buffer = std::make_unique<char[]>(kMaxLogBufferSize);
  snprintf(buffer.get(), kMaxLogBufferSize,
      "[cpp_bus_driver log][%s]->[%s][%u line]: %s", LogLevelName(level),
      file_name, static_cast<unsigned int>(line_number), format);
  vprintf(buffer.get(), args);
  va_end(args);
}

bool Tool::Search(const uint8_t* search_library, size_t search_library_length,
    const char* search_sample, size_t sample_length, size_t* search_index) {
  // 检查参数有效性
  if (search_sample == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (search_library == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (sample_length == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (search_library_length == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (sample_length > search_library_length) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  auto buffer =
      std::search(search_library, search_library + search_library_length,
          search_sample, search_sample + sample_length);
  // 检查是否找到了数据
  if (buffer == (search_library + search_library_length)) {
    return false;
  }

  if (search_index != nullptr) {
    *search_index = buffer - search_library;
  }

  return true;
}

bool Tool::Search(const char* search_library, size_t search_library_length,
    const char* search_sample, size_t sample_length, size_t* search_index) {
  // 检查参数有效性
  if (search_sample == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (search_library == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (sample_length == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (search_library_length == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (sample_length > search_library_length) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  auto buffer =
      std::search(search_library, search_library + search_library_length,
          search_sample, search_sample + sample_length);
  // 检查是否找到了数据
  if (buffer == (search_library + search_library_length)) {
    return false;
  }

  if (search_index != nullptr) {
    *search_index = buffer - search_library;
  }

  return true;
}

bool Tool::SetGpioMode(int32_t pin, GpioMode mode, GpioStatus status) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  if (pin >= static_cast<int32_t>(GPIO_NUM_MAX)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  gpio_config_t config = {};
  config.pin_bit_mask = BIT64(pin);
  switch (mode) {
    case GpioMode::kDisable:
      config.mode = GPIO_MODE_INPUT;
      break;
    case GpioMode::kInput:
      config.mode = GPIO_MODE_INPUT;
      break;
    case GpioMode::kOutput:
      config.mode = GPIO_MODE_OUTPUT;
      break;
    case GpioMode::kOutputOd:
      config.mode = GPIO_MODE_OUTPUT_OD;
      break;
    case GpioMode::kInputOutputOd:
      config.mode = GPIO_MODE_INPUT_OUTPUT_OD;
      break;
    case GpioMode::kInputOutput:
      config.mode = GPIO_MODE_INPUT_OUTPUT;
      break;

    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }
  switch (status) {
    case GpioStatus::kDisable:
      config.pull_up_en = GPIO_PULLUP_DISABLE;
      config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      break;
    case GpioStatus::kPullup:
      config.pull_up_en = GPIO_PULLUP_ENABLE;
      config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      break;
    case GpioStatus::kPulldown:
      config.pull_up_en = GPIO_PULLUP_DISABLE;
      config.pull_down_en = GPIO_PULLDOWN_ENABLE;
      break;

    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }
  config.intr_type = GPIO_INTR_DISABLE;
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
  config.hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE;
#endif

  esp_err_t result = gpio_config(&config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_config failed (error gpio pin: %d, error code: %#X)\n", pin,
        result);
    return false;
  }

  return true;
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  switch (mode) {
    case GpioMode::kDisable:
      nrf_gpio_cfg_default(pin);
      break;
    case GpioMode::kInput:
      switch (status) {
        case GpioStatus::kDisable:
          pinMode(pin, 0x0);
          break;
        case GpioStatus::kPullup:
          pinMode(pin, 0x2);
          break;
        case GpioStatus::kPulldown:
          pinMode(pin, 0x3);
          break;

        default:
          pinMode(pin, 0x0);
          break;
      }
      break;
    case GpioMode::kOutput:
      pinMode(pin, 0x1);
      break;

    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
      nrf_gpio_cfg_default(pin);
      return false;
  }

  return true;
#else
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "GpioMode failed\n");
  return false;
#endif
}

bool Tool::GpioWrite(int32_t pin, bool value) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  if (pin >= static_cast<int32_t>(GPIO_NUM_MAX)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  esp_err_t result = gpio_set_level(static_cast<gpio_num_t>(pin), value);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_set_level failed (error gpio pin: %d, error code: %#X)\n", pin,
        result);
    return false;
  }

  return true;
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  digitalWrite(pin, value);
  return true;
#else
  return false;
#endif
}

bool Tool::GpioRead(int32_t pin) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  if (pin >= static_cast<int32_t>(GPIO_NUM_MAX)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  return gpio_get_level(static_cast<gpio_num_t>(pin));
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  return digitalRead(pin);
#else
  return false;
#endif
}

bool Tool::ResetGpio(int32_t pin) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  if (pin >= static_cast<int32_t>(GPIO_NUM_MAX)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  const gpio_num_t gpio = static_cast<gpio_num_t>(pin);
  esp_err_t result = gpio_sleep_sel_dis(gpio);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_sleep_sel_dis failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_hold_dis(gpio);
  if ((result != ESP_OK) && (result != ESP_ERR_NOT_SUPPORTED)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_hold_dis failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_reset_pin(gpio);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_reset_pin failed (error code: %#X)\n", result);
    return false;
  }

  return true;
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  nrf_gpio_cfg_default(pin);
  return true;
#else
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "ResetGpio failed\n");
  return false;
#endif
}

void Tool::DelayMs(uint32_t value) {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  // 默认状态下 vTaskDelay 在小于 10ms 延时时不精确
  // vTaskDelay(pdMS_TO_TICKS(value));
  usleep(value * 1000);
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  delay(value);
#endif
}

void Tool::DelayUs(uint32_t value) {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  usleep(value);
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  delayMicroseconds(value);
#endif
}

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)

int64_t Tool::GetSystemTimeUs() { return esp_timer_get_time(); }

int64_t Tool::GetSystemTimeMs() { return esp_timer_get_time() / 1000; }

bool Tool::InitGpioInterrupt(
    uint32_t pin, InterruptMode mode, void (*interrupt)(void*), void* args) {
  if (pin >= static_cast<uint32_t>(GPIO_NUM_MAX)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %u)\n", pin);
    return false;
  }

  gpio_config_t config = {};
  config.pin_bit_mask = BIT64(pin);
  config.mode = GPIO_MODE_INPUT;
  switch (mode) {
    case InterruptMode::kDisable:
      config.pull_up_en = GPIO_PULLUP_DISABLE;
      config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      config.intr_type = GPIO_INTR_DISABLE;
      break;
    case InterruptMode::kRising:
      config.pull_up_en = GPIO_PULLUP_DISABLE;
      config.pull_down_en = GPIO_PULLDOWN_ENABLE;
      config.intr_type = GPIO_INTR_POSEDGE;
      break;
    case InterruptMode::kFalling:
      config.pull_up_en = GPIO_PULLUP_ENABLE;
      config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      config.intr_type = GPIO_INTR_NEGEDGE;
      break;
    case InterruptMode::kChange:
      config.pull_up_en = GPIO_PULLUP_DISABLE;
      config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      config.intr_type = GPIO_INTR_ANYEDGE;
      break;
    case InterruptMode::kOnLow:
      // 只要 kGpio 引脚保持低电平，就会持续触发中断
      // 需要确保中断处理函数可以处理这种情况，或外部信号不会长时间保持低电平
      // 否则系统可能崩溃重启
      config.pull_up_en = GPIO_PULLUP_ENABLE;
      config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      config.intr_type = GPIO_INTR_LOW_LEVEL;
      break;
    case InterruptMode::kOnHigh:
      // 只要 kGpio 引脚保持高电平，就会持续触发中断
      // 需要确保中断处理函数可以处理这种情况，或外部信号不会长时间保持高电平
      // 否则系统可能崩溃重启
      config.pull_up_en = GPIO_PULLUP_DISABLE;
      config.pull_down_en = GPIO_PULLDOWN_ENABLE;
      config.intr_type = GPIO_INTR_HIGH_LEVEL;
      break;

    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
  config.hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE;
#endif

  esp_err_t result = gpio_config(&config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_config failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_install_isr_service(0);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_install_isr_service failed (error code: %#X)\n", result);
  }

  result = gpio_isr_handler_add(static_cast<gpio_num_t>(pin), interrupt, args);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_isr_handler_add failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_intr_enable(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_intr_enable failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool Tool::DeinitGpioInterrupt(uint32_t pin) {
  esp_err_t result =
      gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_DISABLE);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_set_intr_type failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_isr_handler_remove failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_intr_disable(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_intr_disable failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_reset_pin(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_reset_pin failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool Pwm::Init(ledc_timer_t timer_num, ledc_channel_t channel, uint32_t freq_hz,
    uint32_t duty, ledc_mode_t speed_mode, ledc_timer_bit_t duty_resolution,
    ledc_sleep_mode_t sleep_mode) {
  const ledc_timer_config_t buffer_ledc_timer_config = {
      .speed_mode = speed_mode,
      .duty_resolution = duty_resolution,  // LEDC 驱动器占空比精度
      // LEDC 使用的定时器编号，若要生成多个频率不同的 PWM 信号，需要指定不同定时器
      .timer_num = timer_num,
      .freq_hz = freq_hz,        // PWM 频率
      .clk_cfg = LEDC_AUTO_CLK,  // 自动选择定时器时钟源
      .deconfigure = false,
  };

  esp_err_t result = ledc_timer_config(&buffer_ledc_timer_config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_timer_config failed (error code: %#X)\n", result);
    return false;
  }

  const ledc_channel_config_t buffer_ledc_channel_config = {
      .gpio_num = pin_,
      .speed_mode = speed_mode,
      .channel = channel,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = timer_num,
      .duty = duty,
      .hpoint = 0,
      .sleep_mode = sleep_mode,
      .flags =
          {
              .output_invert = false,
          },
  };

  result = ledc_channel_config(&buffer_ledc_channel_config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_channel_config failed (error code: %#X)\n", result);
    return false;
  }

  channel_ = channel;
  freq_hz_ = freq_hz;
  duty_ = duty;
  speed_mode_ = speed_mode;
  duty_resolution_ = duty_resolution;
  timer_num_ = timer_num;
  sleep_mode_ = sleep_mode;

  return true;
}

bool Pwm::SetDuty(uint8_t duty) {
  if (duty > 100) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    duty = 100;
  }

  esp_err_t result = ledc_set_duty(speed_mode_, channel_,
      (static_cast<float>(duty) / 100.0) *
          (1 << static_cast<uint8_t>(duty_resolution_)));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_set_duty failed (error code: %#X)\n", result);
    return false;
  }

  result = ledc_update_duty(speed_mode_, channel_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_update_duty failed (error code: %#X)\n", result);
    return false;
  }

  duty_ = duty;

  return true;
}

bool Pwm::SetFrequency(uint32_t freq_hz) {
  esp_err_t result = ledc_set_freq(speed_mode_, timer_num_, freq_hz);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_set_freq failed (error code: %#X)\n", result);
    return false;
  }

  freq_hz_ = freq_hz;

  return true;
}

bool Pwm::StartGradientTime(uint8_t target_duty, int32_t time_ms) {
  if (target_duty > 100) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    target_duty = 100;
  }

  esp_err_t result = ledc_fade_func_install(0);
  if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_fade_func_install failed (error code: %#X)\n", result);
  }

  result = ledc_set_fade_with_time(speed_mode_, channel_,
      (static_cast<float>(target_duty) / 100.0) *
          (1 << static_cast<uint8_t>(duty_resolution_)),
      time_ms);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_set_fade_with_time failed (error code: %#X)\n", result);
    return false;
  }

  result = ledc_fade_start(
      speed_mode_, channel_, ledc_fade_mode_t::LEDC_FADE_WAIT_DONE);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_fade_start failed (error code: %#X)\n", result);
    return false;
  }

  duty_ = target_duty;

  return true;
}

bool Pwm::Stop(uint32_t idle_level) {
  esp_err_t result = ledc_stop(speed_mode_, channel_, idle_level);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ledc_stop failed (error code: %#X)\n", result);
    return false;
  }

  ledc_fade_func_uninstall();

  return true;
}
#endif

namespace {
/**
 * @brief 单条 NMEA 语句拆分后的基础信息
 */
struct NmeaSentenceLine {
  std::string talker_id;
  std::string formatter;
  std::vector<std::string> parts;
};

/**
 * @brief 解析整数类型 NMEA 字段
 * @param field NMEA 字段字符串
 * @param value 输出解析后的整数
 * @return 解析成功返回 true
 */
bool ParseLongField(const std::string& field, long* value) {
  return safe_convert::SafeStringToLong(field, value);
}

/**
 * @brief 解析浮点类型 NMEA 字段
 * @param field NMEA 字段字符串
 * @param value 输出解析后的浮点数
 * @return 解析成功返回 true
 */
bool ParseFloatField(const std::string& field, float* value) {
  return safe_convert::SafeStringToFloat(field, value);
}

/**
 * @brief 解析单个十六进制字符
 * @param input 十六进制字符
 * @param value 输出转换后的数值
 * @return 转换成功返回 true
 */
bool ParseHexDigit(char input, uint8_t* value) {
  if (value == nullptr) {
    return false;
  }

  if (input >= '0' && input <= '9') {
    *value = input - '0';
    return true;
  }
  if (input >= 'A' && input <= 'F') {
    *value = input - 'A' + 10;
    return true;
  }
  if (input >= 'a' && input <= 'f') {
    *value = input - 'a' + 10;
    return true;
  }

  return false;
}

/**
 * @brief 校验 NMEA 语句中的 XOR 校验值
 * @param line 单条 NMEA 语句
 * @param star_pos '*' 字符所在位置
 * @return 校验格式和校验值均正确返回 true
 */
bool ValidateNmeaChecksum(const std::string& line, size_t star_pos) {
  if (star_pos == std::string::npos) {
    return false;
  }
  if (line.size() - star_pos != 3) {
    return false;
  }

  uint8_t high = 0;
  uint8_t low = 0;
  if (!ParseHexDigit(line[star_pos + 1], &high) ||
      !ParseHexDigit(line[star_pos + 2], &low)) {
    return false;
  }

  uint8_t checksum = 0;
  for (size_t i = 1; i < star_pos; ++i) {
    checksum ^= static_cast<uint8_t>(line[i]);
  }

  return checksum == static_cast<uint8_t>((high << 4) | low);
}

/**
 * @brief 按逗号拆分 NMEA 语句字段
 * @param content 去掉校验和后的 NMEA 语句内容
 * @return 拆分后的字段列表
 */
std::vector<std::string> SplitNmeaFields(const std::string& content) {
  std::vector<std::string> parts;
  std::stringstream ss(content);
  std::string token;

  while (std::getline(ss, token, ',')) {
    parts.push_back(token);
  }

  if (!content.empty() && content.back() == ',') {
    parts.push_back("");
  }

  return parts;
}

/**
 * @brief 从原始数据中查找指定 formatter 的 NMEA 语句
 * @param data 原始 NMEA 数据
 * @param length 数据长度
 * @param formatter 三字符语句类型，例如 RMC、GGA
 * @return 匹配到的语句列表
 */
std::vector<NmeaSentenceLine> FindNmeaSentences(
    const uint8_t* data, size_t length, const char* formatter) {
  std::vector<NmeaSentenceLine> lines;
  if (data == nullptr || length == 0 || formatter == nullptr) {
    return lines;
  }

  const std::string received_data(reinterpret_cast<const char*>(data), length);
  size_t start_pos = 0;

  while ((start_pos = received_data.find('$', start_pos)) !=
         std::string::npos) {
    size_t end_pos = received_data.find_first_of("\r\n", start_pos);
    std::string line = (end_pos == std::string::npos)
                           ? received_data.substr(start_pos)
                           : received_data.substr(start_pos, end_pos - start_pos);

    start_pos = (end_pos == std::string::npos) ? received_data.size()
                                               : end_pos + 1;
    if (line.size() < 6) {
      continue;
    }

    const size_t star_pos = line.find('*');
    if (!ValidateNmeaChecksum(line, star_pos)) {
      continue;
    }

    const std::string content =
        (star_pos == std::string::npos) ? line : line.substr(0, star_pos);
    std::vector<std::string> parts = SplitNmeaFields(content);
    if (parts.empty() || parts[0].size() < 6 || parts[0][0] != '$') {
      continue;
    }

    const std::string address = parts[0].substr(1);
    if (address.size() < 5) {
      continue;
    }

    const std::string sentence_formatter =
        address.substr(address.size() - 3);
    if (sentence_formatter != formatter) {
      continue;
    }

    NmeaSentenceLine sentence;
    sentence.talker_id = address.substr(0, address.size() - 3);
    sentence.formatter = sentence_formatter;
    sentence.parts = parts;
    lines.push_back(sentence);
  }

  return lines;
}

/**
 * @brief 解析 UTC 时间字段
 * @param field UTC 时间字段，格式 hhmmss.ss
 * @param utc 输出 UTC 时间结构体
 * @return 解析成功返回 true
 */
template <typename Utc>
bool ParseUtcField(const std::string& field, Utc& utc) {
  if (field.length() < 6) {
    return false;
  }

  long hour = 0;
  long minute = 0;
  float second = 0.0;
  if (!ParseLongField(field.substr(0, 2), &hour) ||
      !ParseLongField(field.substr(2, 2), &minute) ||
      !ParseFloatField(field.substr(4), &second)) {
    return false;
  }
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0.0 ||
      second >= 60.0) {
    return false;
  }

  utc.hour = hour;
  utc.minute = minute;
  utc.second = second;
  utc.update_flag = true;
  return true;
}

/**
 * @brief 解析 NMEA 经纬度字段
 * @param field 经纬度字段，纬度格式 ddmm.mmmm，经度格式 dddmm.mmmm
 * @param direction 方向字段，纬度 N/S，经度 E/W
 * @param degree_digits 度数部分的位数
 * @param coordinate 输出坐标结构体
 * @return 解析成功返回 true
 */
template <typename Coordinate>
bool ParseCoordinateField(
    const std::string& field, const std::string& direction, size_t degree_digits,
    Coordinate& coordinate) {
  if (field.length() < degree_digits + 2) {
    return false;
  }

  long degrees = 0;
  float minutes = 0.0;
  if (!ParseLongField(field.substr(0, degree_digits), &degrees) ||
      !ParseFloatField(field.substr(degree_digits), &minutes)) {
    return false;
  }
  const long max_degrees = (degree_digits == 2) ? 90 : 180;
  if (degrees < 0 || degrees > max_degrees || minutes < 0.0 ||
      minutes >= 60.0) {
    return false;
  }
  if (!direction.empty()) {
    const char direction_value = direction[0];
    if (degree_digits == 2 && direction_value != 'N' &&
        direction_value != 'S') {
      return false;
    }
    if (degree_digits == 3 && direction_value != 'E' &&
        direction_value != 'W') {
      return false;
    }
  }

  coordinate.degrees = degrees;
  coordinate.minutes = minutes;
  coordinate.degrees_minutes =
      static_cast<double>(degrees) + static_cast<double>(minutes / 60.0);
  coordinate.update_flag = true;

  if (!direction.empty()) {
    coordinate.direction.assign(1, direction[0]);
    coordinate.direction_update_flag = true;
  }

  return true;
}

/**
 * @brief 解析一组纬度和经度字段
 * @param parts NMEA 字段列表
 * @param lat_index 纬度字段索引
 * @param lat_direction_index 纬度方向字段索引
 * @param lon_index 经度字段索引
 * @param lon_direction_index 经度方向字段索引
 * @param location 输出位置结构体
 * @return 成功解析到任意坐标返回 true
 */
template <typename Location>
bool ParseLocationFields(const std::vector<std::string>& parts, size_t lat_index,
    size_t lat_direction_index, size_t lon_index, size_t lon_direction_index,
    Location& location) {
  bool parsed = false;

  if (parts.size() > lat_direction_index) {
    parsed |= ParseCoordinateField(
        parts[lat_index], parts[lat_direction_index], 2, location.lat);
  }
  if (parts.size() > lon_direction_index) {
    parsed |= ParseCoordinateField(
        parts[lon_index], parts[lon_direction_index], 3, location.lon);
  }

  return parsed;
}

/**
 * @brief 解析 ddmmyy 日期字段
 * @param field 日期字段
 * @param date 输出日期结构体
 * @return 解析成功返回 true
 */
template <typename Date>
bool ParseDdmmyyField(const std::string& field, Date& date) {
  if (field.length() != 6) {
    return false;
  }

  long day = 0;
  long month = 0;
  long year = 0;
  if (!ParseLongField(field.substr(0, 2), &day) ||
      !ParseLongField(field.substr(2, 2), &month) ||
      !ParseLongField(field.substr(4, 2), &year)) {
    return false;
  }
  if (day < 1 || day > 31 || month < 1 || month > 12 || year < 0 ||
      year > 99) {
    return false;
  }

  date.day = day;
  date.month = month;
  date.year = year;
  date.update_flag = true;
  return true;
}

/**
 * @brief 解析 ZDA 日期字段
 * @param day_field 日字段
 * @param month_field 月字段
 * @param year_field 年字段
 * @param date 输出日期结构体
 * @return 解析成功返回 true
 */
template <typename Date>
bool ParseZdaDateFields(const std::string& day_field,
    const std::string& month_field, const std::string& year_field, Date& date) {
  long day = 0;
  long month = 0;
  long year = 0;
  if (!ParseLongField(day_field, &day) ||
      !ParseLongField(month_field, &month) ||
      !ParseLongField(year_field, &year)) {
    return false;
  }
  if (day < 1 || day > 31 || month < 1 || month > 12 || year < 0 ||
      year > 9999) {
    return false;
  }

  date.day = day;
  date.month = month;
  date.year = year;
  date.update_flag = true;
  return true;
}

/**
 * @brief 解析指定索引处的 uint8_t 字段
 * @param parts NMEA 字段列表
 * @param index 字段索引
 * @param value 输出解析后的数值
 * @return 解析成功返回 true
 */
bool ParseUint8At(
    const std::vector<std::string>& parts, size_t index, uint8_t& value) {
  long parsed = 0;
  if (parts.size() <= index || !ParseLongField(parts[index], &parsed)) {
    return false;
  }
  if (parsed < 0 || parsed > 0xFF) {
    return false;
  }

  value = parsed;
  return true;
}

/**
 * @brief 解析指定索引处的 uint16_t 字段
 * @param parts NMEA 字段列表
 * @param index 字段索引
 * @param value 输出解析后的数值
 * @return 解析成功返回 true
 */
bool ParseUint16At(
    const std::vector<std::string>& parts, size_t index, uint16_t& value) {
  long parsed = 0;
  if (parts.size() <= index || !ParseLongField(parts[index], &parsed)) {
    return false;
  }
  if (parsed < 0 || parsed > 0xFFFF) {
    return false;
  }

  value = parsed;
  return true;
}

/**
 * @brief 解析指定索引处的浮点字段
 * @param parts NMEA 字段列表
 * @param index 字段索引
 * @param value 输出解析后的浮点数
 * @return 解析成功返回 true
 */
bool ParseFloatAt(
    const std::vector<std::string>& parts, size_t index, float& value) {
  if (parts.size() <= index) {
    return false;
  }

  return ParseFloatField(parts[index], &value);
}

/**
 * @brief 读取指定索引处字段的首字符
 * @param parts NMEA 字段列表
 * @param index 字段索引
 * @param value 输出单字符字符串
 * @return 读取成功返回 true
 */
bool AssignCharAt(
    const std::vector<std::string>& parts, size_t index, std::string& value) {
  if (parts.size() <= index || parts[index].empty()) {
    return false;
  }

  value.assign(1, parts[index][0]);
  return true;
}
}  // namespace

bool GnssParser::ParseRmcInfo(const uint8_t* data, size_t length, Rmc& rmc) {
  const std::vector<NmeaSentenceLine> lines =
      FindNmeaSentences(data, length, "RMC");
  bool parsed = false;

  for (const auto& line : lines) {
    const std::vector<std::string>& parts = line.parts;

    if (parts.size() > 1) {
      parsed |= ParseUtcField(parts[1], rmc.utc);
    }
    if (AssignCharAt(parts, 2, rmc.location_status)) {
      rmc.location_status_update_flag = true;
      parsed = true;
    }
    parsed |= ParseLocationFields(parts, 3, 4, 5, 6, rmc.location);
    parsed |= ParseFloatAt(parts, 7, rmc.speed_over_ground_knots);
    parsed |= ParseFloatAt(parts, 8, rmc.course_over_ground_degree);
    if (parts.size() > 9) {
      parsed |= ParseDdmmyyField(parts[9], rmc.data);
    }
    parsed |= ParseFloatAt(parts, 10, rmc.magnetic_variation);
    parsed |= AssignCharAt(parts, 11, rmc.magnetic_variation_direction);
    parsed |= AssignCharAt(parts, 12, rmc.mode_indicator);
    parsed |= AssignCharAt(parts, 13, rmc.navigational_status);
  }

  return parsed;
}

bool GnssParser::ParseGgaInfo(const uint8_t* data, size_t length, Gga& gga) {
  const std::vector<NmeaSentenceLine> lines =
      FindNmeaSentences(data, length, "GGA");
  bool parsed = false;

  for (const auto& line : lines) {
    const std::vector<std::string>& parts = line.parts;

    if (parts.size() > 1) {
      parsed |= ParseUtcField(parts[1], gga.utc);
    }
    parsed |= ParseLocationFields(parts, 2, 3, 4, 5, gga.location);
    parsed |= ParseUint8At(parts, 6, gga.gps_mode_status);
    parsed |= ParseUint8At(parts, 7, gga.online_satellite_count);
    parsed |= ParseFloatAt(parts, 8, gga.hdop);
    parsed |= ParseFloatAt(parts, 9, gga.altitude);
    parsed |= AssignCharAt(parts, 10, gga.altitude_unit);
    parsed |= ParseFloatAt(parts, 11, gga.geoid_separation);
    parsed |= AssignCharAt(parts, 12, gga.geoid_separation_unit);
    parsed |= ParseFloatAt(parts, 13, gga.differential_age);
    if (parts.size() > 14 && !parts[14].empty()) {
      gga.differential_station_id = parts[14];
      parsed = true;
    }
  }

  return parsed;
}

bool GnssParser::ParseGsvInfo(const uint8_t* data, size_t length, Gsv& gsv) {
  const std::vector<NmeaSentenceLine> lines =
      FindNmeaSentences(data, length, "GSV");
  bool parsed = false;

  gsv.satellites.clear();
  for (const auto& line : lines) {
    const std::vector<std::string>& parts = line.parts;

    gsv.talker_id = line.talker_id;
    parsed |= ParseUint8At(parts, 1, gsv.total_sentence_count);
    parsed |= ParseUint8At(parts, 2, gsv.sentence_number);
    parsed |= ParseUint8At(parts, 3, gsv.total_satellite_count);

    size_t repeat_end = parts.size();
    uint8_t signal_id = -1;
    if (parts.size() > 4 && ((parts.size() - 4) % 4) == 1) {
      ParseUint8At(parts, parts.size() - 1, signal_id);
      repeat_end--;
      gsv.signal_id = signal_id;
    }

    for (size_t i = 4; i + 3 < repeat_end; i += 4) {
      Gsv::Satellite satellite;
      satellite.talker_id = line.talker_id;
      satellite.signal_id = signal_id;

      bool satellite_parsed = false;
      satellite_parsed |= ParseUint16At(parts, i, satellite.id);
      long elevation = 0;
      if (ParseLongField(parts[i + 1], &elevation) && elevation >= 0 &&
          elevation <= 90) {
        satellite.elevation = elevation;
        satellite_parsed = true;
      }
      long azimuth = 0;
      if (ParseLongField(parts[i + 2], &azimuth) && azimuth >= 0 &&
          azimuth <= 359) {
        satellite.azimuth = azimuth;
        satellite_parsed = true;
      }
      long cn0 = 0;
      if (ParseLongField(parts[i + 3], &cn0) && cn0 >= 0 && cn0 <= 99) {
        satellite.cn0 = cn0;
        satellite_parsed = true;
      }

      if (satellite_parsed) {
        gsv.satellites.push_back(satellite);
        parsed = true;
      }
    }
  }

  gsv.update_flag = parsed;
  return parsed;
}

bool GnssParser::ParseGsaInfo(const uint8_t* data, size_t length, Gsa& gsa) {
  const std::vector<NmeaSentenceLine> lines =
      FindNmeaSentences(data, length, "GSA");
  bool parsed = false;

  gsa.sentences.clear();
  for (const auto& line : lines) {
    const std::vector<std::string>& parts = line.parts;
    Gsa::Sentence sentence;
    sentence.talker_id = line.talker_id;

    bool sentence_parsed = false;
    sentence_parsed |= AssignCharAt(parts, 1, sentence.selection_mode);
    sentence_parsed |= ParseUint8At(parts, 2, sentence.fix_mode);

    for (size_t i = 3; i <= 14 && i < parts.size(); ++i) {
      uint16_t satellite_id = 0;
      if (ParseUint16At(parts, i, satellite_id)) {
        sentence.satellite_ids.push_back(satellite_id);
        sentence_parsed = true;
      }
    }

    sentence_parsed |= ParseFloatAt(parts, 15, sentence.pdop);
    sentence_parsed |= ParseFloatAt(parts, 16, sentence.hdop);
    sentence_parsed |= ParseFloatAt(parts, 17, sentence.vdop);
    sentence_parsed |= ParseUint8At(parts, 18, sentence.system_id);

    if (sentence_parsed) {
      gsa.sentences.push_back(sentence);
      parsed = true;
    }
  }

  gsa.update_flag = parsed;
  return parsed;
}

bool GnssParser::ParseVtgInfo(const uint8_t* data, size_t length, Vtg& vtg) {
  const std::vector<NmeaSentenceLine> lines =
      FindNmeaSentences(data, length, "VTG");
  bool parsed = false;

  for (const auto& line : lines) {
    const std::vector<std::string>& parts = line.parts;
    parsed |= ParseFloatAt(parts, 1, vtg.course_true_degree);
    parsed |= ParseFloatAt(parts, 3, vtg.course_magnetic_degree);
    parsed |= ParseFloatAt(parts, 5, vtg.speed_knots);
    parsed |= ParseFloatAt(parts, 7, vtg.speed_kmh);
    parsed |= AssignCharAt(parts, 9, vtg.mode_indicator);
  }

  vtg.update_flag = parsed;
  return parsed;
}

bool GnssParser::ParseGllInfo(const uint8_t* data, size_t length, Gll& gll) {
  const std::vector<NmeaSentenceLine> lines =
      FindNmeaSentences(data, length, "GLL");
  bool parsed = false;

  for (const auto& line : lines) {
    const std::vector<std::string>& parts = line.parts;
    parsed |= ParseLocationFields(parts, 1, 2, 3, 4, gll.location);
    if (parts.size() > 5) {
      parsed |= ParseUtcField(parts[5], gll.utc);
    }
    parsed |= AssignCharAt(parts, 6, gll.location_status);
    parsed |= AssignCharAt(parts, 7, gll.mode_indicator);
  }

  gll.update_flag = parsed;
  return parsed;
}

bool GnssParser::ParseTxtInfo(const uint8_t* data, size_t length, Txt& txt) {
  const std::vector<NmeaSentenceLine> lines =
      FindNmeaSentences(data, length, "TXT");
  bool parsed = false;

  txt.sentences.clear();
  for (const auto& line : lines) {
    const std::vector<std::string>& parts = line.parts;
    Txt::Sentence sentence;
    bool sentence_parsed = false;

    sentence_parsed |= ParseUint8At(parts, 1, sentence.total_sentence_count);
    sentence_parsed |= ParseUint8At(parts, 2, sentence.sentence_number);
    sentence_parsed |= ParseUint8At(parts, 3, sentence.text_id);
    if (parts.size() > 4) {
      sentence.text = parts[4];
      for (size_t i = 5; i < parts.size(); ++i) {
        sentence.text += ",";
        sentence.text += parts[i];
      }
      sentence_parsed = true;
    }

    if (sentence_parsed) {
      txt.sentences.push_back(sentence);
      parsed = true;
    }
  }

  txt.update_flag = parsed;
  return parsed;
}

bool GnssParser::ParseZdaInfo(const uint8_t* data, size_t length, Zda& zda) {
  const std::vector<NmeaSentenceLine> lines =
      FindNmeaSentences(data, length, "ZDA");
  bool parsed = false;

  for (const auto& line : lines) {
    const std::vector<std::string>& parts = line.parts;
    if (parts.size() > 1) {
      parsed |= ParseUtcField(parts[1], zda.utc);
    }

    if (parts.size() > 4 &&
        ParseZdaDateFields(parts[2], parts[3], parts[4], zda.date)) {
      parsed = true;
    }

    long value = 0;
    if (parts.size() > 5 && ParseLongField(parts[5], &value) &&
        value >= -13 && value <= 13) {
      zda.local_hour = value;
      parsed = true;
    }
    if (parts.size() > 6 && ParseLongField(parts[6], &value) &&
        value >= 0 && value <= 59) {
      zda.local_minute = value;
      parsed = true;
    }
  }

  zda.update_flag = parsed;
  return parsed;
}

bool GnssParser::ParseInfo(const uint8_t* data, size_t length, Info& info) {
  bool parsed = false;
  parsed |= ParseRmcInfo(data, length, info.rmc);
  parsed |= ParseGgaInfo(data, length, info.gga);
  parsed |= ParseGsvInfo(data, length, info.gsv);
  parsed |= ParseGsaInfo(data, length, info.gsa);
  parsed |= ParseVtgInfo(data, length, info.vtg);
  parsed |= ParseGllInfo(data, length, info.gll);
  parsed |= ParseTxtInfo(data, length, info.txt);
  parsed |= ParseZdaInfo(data, length, info.zda);
  return parsed;
}

}  // namespace cpp_bus_driver
