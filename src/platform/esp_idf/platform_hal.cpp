/*
 * @Description: ESP-IDF 平台硬件抽象实现
 * @Author: LILYGO_L
 * @Date: 2026-09-04 10:14:25
 * @LastEditTime: 2026-09-05 14:57:42
 * @License: GPL 3.0
 */
#include "platform/platform_hal.h"

#include <mutex>

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32

#include "driver/gpio.h"
#include "esp_bit_defs.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "unistd.h"

namespace cpp_bus_driver {
namespace {
std::mutex g_gpio_isr_service_mutex;
bool g_gpio_isr_service_installed = false;

/**
 * @brief 确保全局 GPIO ISR 服务只安装一次
 * @return 安装成功或服务已经存在时返回 ESP_OK，否则返回实际错误码
 */
esp_err_t EnsureGpioIsrServiceInstalled() {
  std::lock_guard<std::mutex> lock(g_gpio_isr_service_mutex);
  if (g_gpio_isr_service_installed) {
    return ESP_OK;
  }

  const esp_err_t result = gpio_install_isr_service(0);
  if (result == ESP_OK || result == ESP_ERR_INVALID_STATE) {
    g_gpio_isr_service_installed = true;
    return ESP_OK;
  }

  return result;
}

}  // namespace

bool PlatformHal::SetGpioMode(int32_t pin, GpioMode mode, GpioStatus status) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

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
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
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
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
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
}

bool PlatformHal::GpioWrite(int32_t pin, bool value) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

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
}

bool PlatformHal::GpioRead(int32_t pin) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  if (pin >= static_cast<int32_t>(GPIO_NUM_MAX)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

  return gpio_get_level(static_cast<gpio_num_t>(pin));
}

bool PlatformHal::ResetGpio(int32_t pin) {
  if (pin < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range (gpio pin: %d)\n", pin);
    return false;
  }

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
}

void PlatformHal::DelayMs(uint32_t value) {
  // 默认状态下 vTaskDelay 在小于 10ms 延时时不精确
  usleep(value * 1000);
}

void PlatformHal::DelayUs(uint32_t value) { usleep(value); }

int64_t PlatformHal::GetSystemTimeUs() const { return esp_timer_get_time(); }

int64_t PlatformHal::GetSystemTimeMs() const {
  return esp_timer_get_time() / 1000;
}

bool PlatformHal::InitGpioInterrupt(uint32_t pin, InterruptMode mode,
    void (*interrupt)(void*), void* args, GpioStatus status) {
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
      config.intr_type = GPIO_INTR_DISABLE;
      break;
    case InterruptMode::kRising:
      config.intr_type = GPIO_INTR_POSEDGE;
      break;
    case InterruptMode::kFalling:
      config.intr_type = GPIO_INTR_NEGEDGE;
      break;
    case InterruptMode::kChange:
      config.intr_type = GPIO_INTR_ANYEDGE;
      break;
    case InterruptMode::kOnLow:
      // 只要 kGpio 引脚保持低电平，就会持续触发中断
      // 需要确保中断处理函数可以处理这种情况，或外部信号不会长时间保持低电平
      // 否则系统可能崩溃重启
      config.intr_type = GPIO_INTR_LOW_LEVEL;
      break;
    case InterruptMode::kOnHigh:
      // 只要 kGpio 引脚保持高电平，就会持续触发中断
      // 需要确保中断处理函数可以处理这种情况，或外部信号不会长时间保持高电平
      // 否则系统可能崩溃重启
      config.intr_type = GPIO_INTR_HIGH_LEVEL;
      break;

    default:
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
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
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
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

  result = EnsureGpioIsrServiceInstalled();
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_install_isr_service failed (error code: %#X)\n", result);
    return false;
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

    // 中断使能失败时撤销已经注册的处理函数，避免留下半初始化状态。
    esp_err_t cleanup_result =
        gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
    if (cleanup_result != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "gpio_isr_handler_remove failed (error code: %#X)\n", cleanup_result);
    }
    cleanup_result = gpio_reset_pin(static_cast<gpio_num_t>(pin));
    if (cleanup_result != ESP_OK) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "gpio_reset_pin failed (error code: %#X)\n", cleanup_result);
    }
    return false;
  }

  return true;
}

bool PlatformHal::DeinitGpioInterrupt(uint32_t pin) {
  bool deinit_ok = true;
  esp_err_t result = gpio_intr_disable(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_intr_disable failed (error code: %#X)\n", result);
    deinit_ok = false;
  }

  result = gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_DISABLE);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_set_intr_type failed (error code: %#X)\n", result);
    deinit_ok = false;
  }

  // 即使前面的硬件操作失败，也继续移除 ISR，避免回调参数悬空。
  result = gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_isr_handler_remove failed (error code: %#X)\n", result);
    deinit_ok = false;
  }

  result = gpio_reset_pin(static_cast<gpio_num_t>(pin));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "gpio_reset_pin failed (error code: %#X)\n", result);
    deinit_ok = false;
  }

  return deinit_ok;
}

}  // namespace cpp_bus_driver

#endif
