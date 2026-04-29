/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 10:22:46
 * @LastEditTime: 2026-04-24 17:53:48
 * @License: GPL 3.0
 */
#include "tool.h"

namespace cpp_bus_driver {
void Tool::LogMessage(LogLevel level, const char* file_name, size_t line_number,
    const char* format, ...) {
#if defined CPP_BUS_DRIVER_LOG_LEVEL_BUS ||  \
    defined CPP_BUS_DRIVER_LOG_LEVEL_CHIP || \
    defined CPP_BUS_DRIVER_LOG_LEVEL_INFO || \
    defined CPP_BUS_DRIVER_LOG_LEVEL_DEBUG

  switch (level) {
#if defined CPP_BUS_DRIVER_LOG_LEVEL_DEBUG
    case LogLevel::kDebug: {
      va_list args;
      va_start(args, format);
      auto buffer = std::make_unique<char[]>(kMaxLogBufferSize);
      snprintf(buffer.get(), kMaxLogBufferSize,
          "[cpp_bus_driver log][Debug]->[%s][%u line]: %s", file_name,
          line_number, format);
      vprintf(buffer.get(), args);
      va_end(args);

      break;
    }
#endif
#if defined CPP_BUS_DRIVER_LOG_LEVEL_INFO
    case LogLevel::kInfo: {
      va_list args;
      va_start(args, format);
      auto buffer = std::make_unique<char[]>(kMaxLogBufferSize);
      snprintf(buffer.get(), kMaxLogBufferSize,
          "[cpp_bus_driver log][Info]->[%s][%u line]: %s", file_name,
          line_number, format);
      vprintf(buffer.get(), args);
      va_end(args);

      break;
    }
#endif
#if defined CPP_BUS_DRIVER_LOG_LEVEL_BUS
    case LogLevel::kBus: {
      va_list args;
      va_start(args, format);
      auto buffer = std::make_unique<char[]>(kMaxLogBufferSize);
      snprintf(buffer.get(), kMaxLogBufferSize,
          "[cpp_bus_driver log][Bus]->[%s][%u line]: %s", file_name,
          line_number, format);
      vprintf(buffer.get(), args);
      va_end(args);

      break;
    }
#endif
#if defined CPP_BUS_DRIVER_LOG_LEVEL_CHIP
    case LogLevel::kChip: {
      va_list args;
      va_start(args, format);
      auto buffer = std::make_unique<char[]>(kMaxLogBufferSize);
      snprintf(buffer.get(), kMaxLogBufferSize,
          "[cpp_bus_driver log][Chip]->[%s][%u line]: %s", file_name,
          line_number, format);
      vprintf(buffer.get(), args);
      va_end(args);

      break;
    }
#endif
    default:
      break;
  }

#endif
}

bool Tool::Search(const uint8_t* search_library, size_t search_library_length,
    const char* search_sample, size_t sample_length, size_t* search_index) {
  // 检查参数有效性
  if (search_sample == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (search_library == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (sample_length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (search_library_length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (sample_length > search_library_length) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
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
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (search_library == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  } else if (sample_length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (search_library_length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  } else if (sample_length > search_library_length) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
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

bool Tool::SetGpioMode(uint32_t pin, GpioMode mode, GpioStatus status) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  gpio_config_t config;
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
      break;
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
      break;
  }
  config.intr_type = GPIO_INTR_DISABLE;
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
  config.hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE;
#endif

  esp_err_t result = gpio_config(&config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "gpio_config failed (error code: %#X)\n", result);
    return false;
  }

  return true;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
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
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      nrf_gpio_cfg_default(pin);
      return false;
  }

  return true;
#else
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioMode failed\n");
  return false;
#endif
}

bool Tool::GpioWrite(uint32_t pin, bool value) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  esp_err_t result = gpio_set_level(static_cast<gpio_num_t>(pin), value);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "gpio_set_level failed (error code: %#X)\n", result);
    return false;
  }

  return true;
#elif define CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  digitalWrite(pin, value);
  return true;
#else
  return false;
#endif
}

bool Tool::GpioRead(uint32_t pin) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  return gpio_get_level(static_cast<gpio_num_t>(pin));
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  return digitalRead(pin);
#else
  return false;
#endif
}

void Tool::DelayMs(uint32_t value) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  // 默认状态下vTaskDelay在小于10ms延时的时候不精确
  // vTaskDelay(pdMS_TO_TICKS(value));
  usleep(value * 1000);
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  delay(value);
#endif
}

void Tool::DelayUs(uint32_t value) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  usleep(value);
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  delayMicroseconds(value);
#endif
}

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF

int64_t Tool::GetSystemTimeUs() { return esp_timer_get_time(); }

int64_t Tool::GetSystemTimeMs() { return esp_timer_get_time() / 1000; }

bool Tool::InitGpioInterrupt(
    uint32_t pin, InterruptMode mode, void (*interrupt)(void*), void* args) {
  gpio_config_t config;
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
      // 需要确保你的中断处理函数能够处理这种情况，或者你的外部信号不会长时间保持低电平
      // 否则系统将崩溃重启
      config.pull_up_en = GPIO_PULLUP_ENABLE;
      config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      config.intr_type = GPIO_INTR_LOW_LEVEL;
      break;
    case InterruptMode::kOnHigh:
      // 只要 kGpio 引脚保持高电平，就会持续触发中断
      // 需要确保你的中断处理函数能够处理这种情况，或者你的外部信号不会长时间保持高电平
      // 否则系统将崩溃重启
      config.pull_up_en = GPIO_PULLUP_DISABLE;
      config.pull_down_en = GPIO_PULLDOWN_ENABLE;
      config.intr_type = GPIO_INTR_HIGH_LEVEL;
      break;

    default:
      break;
  }
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
  config.hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE;
#endif

  esp_err_t result = gpio_config(&config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "gpio_config failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_install_isr_service(0);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "gpio_install_isr_service failed (error code: %#X)\n", result);
  }

  result = gpio_isr_handler_add(static_cast<gpio_num_t>(pin), interrupt, args);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "gpio_isr_handler_add failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_intr_enable(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "gpio_intr_enable failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool Tool::DeinitGpioInterrupt(uint32_t pin) {
  esp_err_t result =
      gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_DISABLE);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "gpio_set_intr_type failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "gpio_isr_handler_remove failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_intr_disable(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "gpio_intr_disable failed (error code: %#X)\n", result);
    return false;
  }

  result = gpio_reset_pin(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
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
      .duty_resolution = duty_resolution,  // LEDC驱动器占空比精度
      // ledc使用的定时器编号，若需要生成多个频率不同的PWM信号，则需要指定不同的定时器
      .timer_num = timer_num,
      .freq_hz = freq_hz,        // PWM频率
      .clk_cfg = LEDC_AUTO_CLK,  // 自动选择定时器的时钟源
      .deconfigure = false,
  };

  esp_err_t result = ledc_timer_config(&buffer_ledc_timer_config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
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
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
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
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    duty = 100;
  }

  esp_err_t result = ledc_set_duty(speed_mode_, channel_,
      (static_cast<float>(duty) / 100.0) *
          (1 << static_cast<uint8_t>(duty_resolution_)));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "ledc_set_duty failed (error code: %#X)\n", result);
    return false;
  }

  result = ledc_update_duty(speed_mode_, channel_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "ledc_update_duty failed (error code: %#X)\n", result);
    return false;
  }

  duty_ = duty;

  return true;
}

bool Pwm::SetFrequency(uint32_t freq_hz) {
  esp_err_t result = ledc_set_freq(speed_mode_, timer_num_, freq_hz);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "ledc_set_freq failed (error code: %#X)\n", result);
    return false;
  }

  freq_hz_ = freq_hz;

  return true;
}

bool Pwm::StartGradientTime(uint8_t target_duty, int32_t time_ms) {
  if (target_duty > 100) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    target_duty = 100;
  }

  esp_err_t result = ledc_fade_func_install(0);
  if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "ledc_fade_func_install failed (error code: %#X)\n", result);
  }

  result = ledc_set_fade_with_time(speed_mode_, channel_,
      (static_cast<float>(target_duty) / 100.0) *
          (1 << static_cast<uint8_t>(duty_resolution_)),
      time_ms);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "ledc_set_fade_with_time failed (error code: %#X)\n", result);
    return false;
  }

  result = ledc_fade_start(
      speed_mode_, channel_, ledc_fade_mode_t::LEDC_FADE_WAIT_DONE);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "ledc_fade_start failed (error code: %#X)\n", result);
    return false;
  }

  duty_ = target_duty;

  return true;
}

bool Pwm::Stop(uint32_t idle_level) {
  esp_err_t result = ledc_stop(speed_mode_, channel_, idle_level);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "ledc_stop failed (error code: %#X)\n", result);
    return false;
  }

  ledc_fade_func_uninstall();

  return true;
}
#endif

/**
 * @brief 安全的字符串转整数
 * @param &input 输入需要转换的字符串
 * @param *output 输出转换后的数据
 * @return
 * @Date 2026-01-21 09:50:30
 */
bool Tool::SafeStoi(const std::string& input, long* output) {
  if (input.empty()) {
    LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "safe_stoi input is empty\n");
    return false;
  }

  if (output == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  char* endptr = nullptr;
  // strtol 会尝试转换，并将停止位置存入 endptr
  long val = std::strtol(input.c_str(), &endptr, 10);

  // 检查是否发生了有效转换
  // 如果 endptr 依然指向字符串起始位置，说明输入如 "abc"
  if (endptr == input.c_str()) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "safe_stoi conversion failed (error Input: %s)\n", input.c_str());
    return false;
  }

  *output = val;

  return true;
}

/**
 * @brief 安全的字符串转浮点数
 * @param &input 需要转换的字符串
 * @param *output 输出转换后的数据
 * @return
 * @Date 2026-01-21 09:50:30
 */
bool Tool::SafeStof(const std::string& input, float* output) {
  if (input.empty()) {
    LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "safe_stof input is empty\n");
    return false;
  }

  if (output == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  char* endptr = nullptr;
  float val = std::strtof(input.c_str(), &endptr);

  // 如果转换没能开始，或者输入的第一个字符就非法
  if (endptr == input.c_str()) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "safe_stof conversion failed (error Input: %s)\n", input.c_str());
    return false;
  }

  *output = val;

  return true;
}

/**
 * @brief 安全的字符串转双精度浮点数
 * @param &input 需要转换的字符串
 * @param *output 输出转换后的数据
 * @return
 * @Date 2026-01-21 11:58:00
 */
bool Tool::SafeStod(const std::string& input, double* output) {
  if (input.empty()) {
    LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "safe_stod input is empty\n");
    return false;
  }

  if (output == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  char* endptr = nullptr;
  // 使用 std::strtod 处理 double 类型
  double val = std::strtod(input.c_str(), &endptr);

  // 如果转换没能开始（例如输入是非数字字符），或者 endptr 指向字符串开头
  if (endptr == input.c_str()) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "safe_stod conversion failed (error Input: %s)\n", input.c_str());
    return false;
  }

  *output = val;

  return true;
}

bool Gnss::ParseRmcInfo(const uint8_t* data, size_t length, Rmc& rmc) {
  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "ParseRmcInfo(length: %d): \n---begin---\n%s\n---end---\n", length, data);

  // 将接收到的数据转为字符串（即使末尾被截断也能处理）
  std::string received_data(reinterpret_cast<const char*>(data), length);
  std::string target = "$GNRMC";
  size_t start_pos = 0;

  bool exit_flag = false;

  // 循环搜索数据中可能存在的所有 $GNRMC 命令
  while ((start_pos = received_data.find(target, start_pos)) !=
         std::string::npos) {
    // 寻找当前行的结束符，如果找不到说明被截断了，解析到字符串末尾
    size_t end_pos = received_data.find("\r\n", start_pos);
    std::string line;
    if (end_pos == std::string::npos) {
      // 数据被截断
      line = received_data.substr(start_pos);

      LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "The data has been truncated\n");
    } else {
      line = received_data.substr(start_pos, end_pos - start_pos);
    }

    start_pos = end_pos + 2;

    // 处理校验和：寻找 '*' 号
    size_t star_pos = line.find('*');
    // 这里暂时不校验逻辑，只按需截取有效内容进行解析
    std::string content =
        (star_pos != std::string::npos) ? line.substr(0, star_pos) : line;

    std::stringstream ss(content);
    std::string token;
    std::vector<std::string> parts;

    // 按 ',' 分割字符串
    while (std::getline(ss, token, ',')) {
      parts.push_back(token);
    }

    // 基础安全检查：如果分割出的字段少于 2，跳过（防止只有个头）
    if (parts.size() < 2) {
      LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "Insufficient data input\n");
      continue;
    }

    // 开始安全解析各个字段

    // Data 1: kUtc Time (hhmmss.ss 或 hhmmss.sss)
    if (parts.size() > 1 && parts[1].length() >= 6) {
      long buffer_hour, buffer_minute = 0;
      float buffer_second = 0.0;

      if (SafeStoi(parts[1].substr(0, 2), &buffer_hour)) {
        if (SafeStoi(parts[1].substr(2, 2), &buffer_minute)) {
          if (SafeStof(
                  parts[1].substr(4), &buffer_second))  // 自动处理 .ss 或 .sss
          {
            rmc.utc.hour = buffer_hour;
            rmc.utc.minute = buffer_minute;
            rmc.utc.second = buffer_second;

            rmc.utc.update_flag = true;

            exit_flag = true;
          }
        }
      }
    }

    // Data 2: Status (A=有效, V=无效)
    if (parts.size() > 2 && !parts[2].empty()) {
      rmc.location_status = parts[2][0];

      exit_flag = true;
    }

    // Data 3 & 4: Lat (ddmm.mmmm) & N/S
    if (parts.size() > 4 && parts[3].length() >= 4) {
      long buffer_degrees = 0;
      float buffer_minutes = 0.0;

      if (SafeStoi(parts[3].substr(0, 2), &buffer_degrees)) {
        if (SafeStof(parts[3].substr(2), &buffer_minutes)) {
          rmc.location.lat.degrees = buffer_degrees;
          rmc.location.lat.minutes = buffer_minutes;
          rmc.location.lat.degrees_minutes =
              static_cast<double>(rmc.location.lat.degrees) +
              static_cast<double>(rmc.location.lat.minutes / 60.0);
          rmc.location.lat.update_flag = true;

          if (!parts[4].empty()) {
            rmc.location.lat.direction = parts[4][0];
            rmc.location.lat.direction_update_flag = true;
          }

          exit_flag = true;
        }
      }
    }

    // Data 5 & 6: Lon (dddmm.mmmm) & E/W
    if (parts.size() > 6 && parts[5].length() >= 5) {
      long buffer_degrees = 0;
      float buffer_minutes = 0.0;

      if (SafeStoi(parts[5].substr(0, 3), &buffer_degrees)) {
        if (SafeStof(parts[5].substr(3), &buffer_minutes)) {
          rmc.location.lon.degrees = buffer_degrees;
          rmc.location.lon.minutes = buffer_minutes;
          rmc.location.lon.degrees_minutes =
              static_cast<double>(rmc.location.lon.degrees) +
              static_cast<double>(rmc.location.lon.minutes / 60.0);
          rmc.location.lon.update_flag = true;

          if (!parts[6].empty()) {
            rmc.location.lon.direction = parts[6][0];
            rmc.location.lon.direction_update_flag = true;
          }

          exit_flag = true;
        }
      }
    }

    // Data 9: Date (ddmmyy)
    if (parts.size() > 9 && parts[9].length() == 6) {
      long buffer_day = 0, buffer_month = 0, buffer_year = 0;

      if (SafeStoi(parts[9].substr(0, 2), &buffer_day)) {
        if (SafeStoi(parts[9].substr(2, 2), &buffer_month)) {
          if (SafeStoi(parts[9].substr(4, 2), &buffer_year)) {
            rmc.data.day = buffer_day;
            rmc.data.month = buffer_month;
            rmc.data.year = buffer_year;
            rmc.data.update_flag = true;

            exit_flag = true;
          }
        }
      }
    }

    if (exit_flag) {
      LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
          "parse_rmc_info finish (success parse size: %d)\n", parts.size());
      break;
    }
  }

  return true;
}

bool Gnss::ParseGgaInfo(const uint8_t* data, size_t length, Gga& gga) {
  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "ParseGgaInfo(length: %d): \n---begin---\n%s\n---end---\n", length, data);

  std::string received_data(reinterpret_cast<const char*>(data), length);
  std::string target = "$GNGGA";
  size_t start_pos = 0;

  bool exit_flag = false;

  while ((start_pos = received_data.find(target, start_pos)) !=
         std::string::npos) {
    size_t end_pos = received_data.find("\r\n", start_pos);
    std::string line;
    if (end_pos == std::string::npos) {
      // 数据被截断
      line = received_data.substr(start_pos);

      LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "The data has been truncated\n");
    } else {
      line = received_data.substr(start_pos, end_pos - start_pos);
    }

    start_pos = end_pos + 2;

    size_t star_pos = line.find('*');
    std::string content =
        (star_pos != std::string::npos) ? line.substr(0, star_pos) : line;

    std::stringstream ss(content);
    std::string token;
    std::vector<std::string> parts;
    while (std::getline(ss, token, ',')) {
      parts.push_back(token);
    }

    if (parts.size() < 2) {
      LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "Insufficient data input\n");
      continue;
    }

    // Data 1: kUtc Time (hhmmss.ss 或 hhmmss.sss)
    if (parts.size() > 1 && parts[1].length() >= 6) {
      long buffer_hour = 0, buffer_minute = 0;
      float buffer_second = 0.0;

      if (SafeStoi(parts[1].substr(0, 2), &buffer_hour)) {
        if (SafeStoi(parts[1].substr(2, 2), &buffer_minute)) {
          if (SafeStof(parts[1].substr(4), &buffer_second)) {
            gga.utc.hour = buffer_hour;
            gga.utc.minute = buffer_minute;
            gga.utc.second = buffer_second;
            gga.utc.update_flag = true;

            exit_flag = true;
          }
        }
      }
    }

    // Data 2 & 3: Lat (dddmm.mmmm) & N/S
    if (parts.size() > 3 && parts[2].length() >= 4) {
      long buffer_degrees = 0;
      float buffer_minutes = 0.0;

      if (SafeStoi(parts[2].substr(0, 2), &buffer_degrees)) {
        if (SafeStof(parts[2].substr(2), &buffer_minutes)) {
          gga.location.lat.degrees = buffer_degrees;
          gga.location.lat.minutes = buffer_minutes;
          gga.location.lat.degrees_minutes =
              static_cast<double>(gga.location.lat.degrees) +
              static_cast<double>(gga.location.lat.minutes / 60.0);
          gga.location.lat.update_flag = true;

          if (!parts[3].empty()) {
            gga.location.lat.direction = parts[3][0];
            gga.location.lat.direction_update_flag = true;
          }

          exit_flag = true;
        }
      }
    }

    // Data 4 & 5: Lon (dddmm.mmmm) & E/W
    if (parts.size() > 5 && parts[4].length() >= 5) {
      long buffer_degrees = 0;
      float buffer_minutes = 0.0;

      if (SafeStoi(parts[4].substr(0, 3), &buffer_degrees)) {
        if (SafeStof(parts[4].substr(3), &buffer_minutes)) {
          gga.location.lon.degrees = buffer_degrees;
          gga.location.lon.minutes = buffer_minutes;
          gga.location.lon.degrees_minutes =
              static_cast<double>(gga.location.lon.degrees) +
              static_cast<double>(gga.location.lon.minutes / 60.0);
          gga.location.lon.update_flag = true;

          if (!parts[5].empty()) {
            gga.location.lon.direction = parts[5][0];
            gga.location.lon.direction_update_flag = true;
          }

          exit_flag = true;
        }
      }
    }

    // Data 6: Quality
    if (parts.size() > 6) {
      long buffer_gps_mode = 0;

      if (SafeStoi(parts[6], &buffer_gps_mode)) {
        gga.gps_mode_status = buffer_gps_mode;
        exit_flag = true;
      }
    }

    // Data 7: NumSatUsed
    if (parts.size() > 7) {
      long buffer_satellite_count = 0;

      if (SafeStoi(parts[7], &buffer_satellite_count)) {
        gga.online_satellite_count = buffer_satellite_count;
        exit_flag = true;
      }
    }

    // Data 8: kHdop
    if (parts.size() > 8) {
      float buffer_hdop = 0.0;

      if (SafeStof(parts[8], &buffer_hdop)) {
        gga.hdop = buffer_hdop;
        exit_flag = true;
      }
    }

    if (exit_flag) {
      LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
          "parse_gga_info finish (success parse size: %d)\n", parts.size());
      break;
    }
  }

  return true;
}
}  // namespace cpp_bus_driver