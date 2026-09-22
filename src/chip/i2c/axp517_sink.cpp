/*
 * @Description: AXP517 PD Sink 与 PPS 协商状态机实现
 * @Author: LILYGO_L
 * @Date: 2026-09-22 14:14:36
 * @LastEditTime: 2026-09-22 15:11:24
 * @License: GPL 3.0
 */
#include "chip/i2c/axp517.h"

#include <algorithm>

namespace cpp_bus_driver {
namespace {
// PPS 每 5 秒续约，给 10 秒超时预留处理时间。
constexpr uint64_t kPpsRefreshMs = 5000;
// PS_RDY 后给 PMIC VBUS ADC 留出有限的稳定时间；期间维持保守限流。
constexpr uint64_t kVbusSettleTimeoutMs = 1000;
constexpr uint16_t kTxAlerts = 0x0070;

bool IsValidConfig(const Axp517Sink::Config& config) {
  return config.max_input_voltage_mv >= 5000 &&
         config.max_input_voltage_mv <= 15000 &&
         config.preferred_voltage_mv >= 5000 &&
         config.preferred_voltage_mv <= config.max_input_voltage_mv &&
         config.max_input_current_ma >= 500 &&
         config.max_input_current_ma <= 3250 &&
         config.fallback_input_current_ma >= 100 &&
         config.fallback_input_current_ma <= config.max_input_current_ma &&
         config.fallback_vindpm_mv >= 3600 &&
         config.fallback_vindpm_mv <= 5000 &&
         (!config.enable_pps || config.max_input_voltage_mv >= 5100) &&
         (!config.manage_charge_current ||
          (config.contract_charge_current_ma <= 5120 &&
           config.fallback_charge_current_ma <= 5120));
}

bool IsSinkRp(Axp517::CcState state) {
  return state == Axp517::CcState::kRpDefault ||
         state == Axp517::CcState::kRp1500Ma ||
         state == Axp517::CcState::kRp3000Ma;
}
}  // namespace

Axp517Sink::Axp517Sink(Axp517& chip, const Config& config)
    : chip_(chip), config_(config), config_valid_(IsValidConfig(config)),
      contract_charge_current_ma_(config.contract_charge_current_ma) {
  status_.charge_current_managed = config.manage_charge_current;
  if (!config_valid_) {
    logger_.LogMessage(Logger::LogLevel::kError, __FILE__, __LINE__,
        "AXP517 Sink board power policy is invalid\n");
  }
}

const Axp517Sink::Status& Axp517Sink::status() const { return status_; }

bool Axp517Sink::LimitCharging() {
  const uint16_t fallback_charge_ma =
      std::min(config_.fallback_charge_current_ma,
          contract_charge_current_ma_);
  status_.pps = false;
  status_.voltage_mv = 0;
  status_.current_ma = 0;
  status_.charge_current_ma = config_.manage_charge_current
                                  ? fallback_charge_ma : 0;
  // 先降低电池电流，再改变输入限制；不以软件合同代替实际电源状态。
  const bool charge_ok = !config_.manage_charge_current ||
      chip_.SetChargeCurrent(fallback_charge_ma);
  const bool input_ok =
      chip_.SetInputCurrentLimit(config_.fallback_input_current_ma);
  const bool voltage_ok =
      chip_.ApplyNegotiatedInputVoltage(5000, config_.fallback_vindpm_mv);
  return charge_ok && input_ok && voltage_ok;
}

bool Axp517Sink::Send(
    uint8_t type, uint64_t now_ms, const uint32_t* object, uint8_t count) {
  if (tx_pending_) return false;
  Axp517::PdMessage message;
  message.header = type | (static_cast<uint16_t>(revision_) << 6) |
                   (static_cast<uint16_t>(message_id_) << 9);
  if (object != nullptr) {
    if (count == 0 || count > message.data_objects.size()) return false;
    message.header |= static_cast<uint16_t>(count) << 12;
    message.data_object_count = count;
    std::copy_n(object, count, message.data_objects.begin());
  }
  if (!chip_.ClearPdAlerts(kTxAlerts) ||
      !chip_.TransmitPdMessage(
          Axp517::PdTransmitType::kSop, &message, revision_))
    return false;
  if (type == 2 && object != nullptr) ++status_.request_count;
  tx_pending_ = true;
  tx_deadline_ms_ = now_ms + 100;
  return true;
}

bool Axp517Sink::RequestPower(uint64_t now_ms) {
  uint8_t position = 0;
  uint8_t selected_priority = 0;
  requested_voltage_mv_ = 0;
  requested_current_ma_ = 0;
  requested_pps_ = false;
  // 仅考虑板级额定值以内的 SPR 固定 PDO/PPS APDO；不自动推断整板能力。
  for (uint8_t i = 0; i < capabilities_.data_object_count; ++i) {
    const uint32_t pdo = capabilities_.data_objects[i];
    uint16_t voltage = 0;
    uint16_t current = 0;
    const bool pps = (pdo & 0xF8000000) == 0xC0000000;
    if (pps && config_.enable_pps && !pps_rejected_ &&
        revision_ == Axp517::PdRevision::kRev30) {
      const uint16_t minimum = ((pdo >> 8) & 0xFF) * 100;
      const uint16_t maximum = std::min<uint16_t>(
          ((pdo >> 17) & 0xFF) * 100, config_.max_input_voltage_mv);
      if (maximum < minimum) continue;
      voltage = std::max<uint16_t>(minimum,
          std::min<uint16_t>(maximum, config_.preferred_voltage_mv));
      voltage = (voltage / 20) * 20;
      if (minimum > voltage || voltage < 5000) continue;
      current = std::min<uint16_t>((pdo & 0x7F) * 50,
          config_.max_input_current_ma);
      current = (current / 50) * 50;
    } else if ((pdo & 0xC0000000) == 0) {
      voltage = ((pdo >> 10) & 0x3FF) * 50;
      if (voltage < 5000 || voltage > config_.max_input_voltage_mv) continue;
      current = std::min<uint16_t>((pdo & 0x3FF) * 10,
          config_.max_input_current_ma);
      current = (current / 10) * 10;
    } else {
      continue;
    }
    if (current < 500) continue;
    // 在板级额定值内优先选可用功率；相同功率再按固定档/PPS 偏好选。
    // 这样弱 12 V 档不会压过功率更高的 9 V 档。
    const uint32_t power_mw = uint32_t{voltage} * current / 1000;
    const uint32_t selected_power_mw =
        uint32_t{requested_voltage_mv_} * requested_current_ma_ / 1000;
    const uint8_t priority = pps == config_.prefer_pps ? 2 : 1;
    const uint16_t distance_mv = voltage >= config_.preferred_voltage_mv
        ? voltage - config_.preferred_voltage_mv
        : config_.preferred_voltage_mv - voltage;
    const uint16_t selected_distance_mv =
        requested_voltage_mv_ >= config_.preferred_voltage_mv
            ? requested_voltage_mv_ - config_.preferred_voltage_mv
            : config_.preferred_voltage_mv - requested_voltage_mv_;
    if (position != 0 &&
        (power_mw < selected_power_mw ||
         (power_mw == selected_power_mw && priority < selected_priority) ||
         (power_mw == selected_power_mw && priority == selected_priority &&
          (distance_mv > selected_distance_mv ||
           (distance_mv == selected_distance_mv &&
            voltage <= requested_voltage_mv_)))))
      continue;
    position = i + 1;
    selected_priority = priority;
    requested_voltage_mv_ = voltage;
    requested_current_ma_ = current;
    requested_pps_ = pps;
  }
  if (position == 0) {
    logger_.LogMessage(Logger::LogLevel::kWarning, __FILE__, __LINE__,
        "PD source has no compatible SPR fixed/PPS supply\n");
    return false;
  }
  request_ = (uint32_t{position} << 28) | 0x01000000;
  if (requested_pps_) {
    request_ |= (static_cast<uint32_t>(requested_voltage_mv_ / 20) << 9) |
                (requested_current_ma_ / 50);
  } else {
    request_ |= (static_cast<uint32_t>(requested_current_ma_ / 10) << 10) |
                (requested_current_ma_ / 10);
  }
  // 电源切换之前降低负载，PS_RDY 之后才恢复充电电流。
  if (!LimitCharging() || !Send(2, now_ms, &request_)) return false;
  status_.requested_pdo = position;
  status_.requested_voltage_mv = requested_voltage_mv_;
  status_.requested_current_ma = requested_current_ma_;
  status_.requested_pps = requested_pps_;
  status_.state = State::kWaitingAccept;
  deadline_ms_ = now_ms + 100;
  return true;
}

bool Axp517Sink::Fail(bool transmit) {
  status_.failure_stage = status_.state;
  logger_.LogMessage(Logger::LogLevel::kWarning, __FILE__, __LINE__,
      "PD negotiation stopped (state: %u, hard reset: %d)\n",
      static_cast<unsigned>(status_.state), transmit);
  LimitCharging();
  tx_pending_ = false;
  message_id_ = 0;
  received_id_ = -1;
  capabilities_ = {};
  queried_capabilities_ = false;
  // 即使不重试，也要尽力撤销高压合同，不能只关闭 PD 接收。
  if (transmit) {
    chip_.TransmitPdMessage(Axp517::PdTransmitType::kHardReset, nullptr);
  }
  chip_.SetPdReceiveMask(0);
  active_ = false;
  status_.state = State::kError;
  return false;
}

bool Axp517Sink::Receive(const Axp517::PdMessage& message, uint64_t now_ms) {
  if (message.frame_type != Axp517::PdTransmitType::kSop) return true;
  ++status_.rx_count;
  const uint8_t type = message.header & 0x1F;
  const bool control = message.data_object_count == 0;
  const bool extended = (message.header & 0x8000) != 0;
  if (!control && !extended && type == 1) ++status_.source_caps_count;
  if (control && !extended && type == 3) ++status_.accept_count;
  if (control && !extended && type == 6) ++status_.ps_ready_count;
  if (control && !extended && type == 6 &&
      status_.state != State::kWaitingPowerReady &&
      !status_.first_ps_ready_seen) {
    status_.first_ps_ready_seen = true;
    status_.first_ps_ready_pdo = status_.requested_pdo;
    status_.first_ps_ready_target_mv = requested_voltage_mv_;
    status_.first_ps_ready_result = PsReadyResult::kUnexpectedState;
  }
  const int id = (message.header >> 9) & 7;
  if (control && !extended && type == 13) {
    // Soft_Reset 重新同步双方消息编号，接受后等待新的电源能力。
    tx_pending_ = false;
    message_id_ = 0;
    received_id_ = -1;
    if (!LimitCharging() || !Send(3, now_ms)) return false;
    status_.state = State::kWaitingCapabilities;
    deadline_ms_ = now_ms + 500;
    return true;
  }
  if (id == received_id_) return true;
  received_id_ = id;
  const uint8_t revision = (message.header >> 6) & 3;
  if (revision < 1 || revision > 2) return false;
  revision_ = static_cast<Axp517::PdRevision>(revision);
  if (!chip_.SetPdMessageHeader(false, false, revision_)) return false;
  if (extended) return Send(16, now_ms);  // Not_Supported。
  if (!control && type == 1) {
    // 首个电源能力必须是 5 V 固定电源，不能将任意对象当作安全回退。
    const uint32_t first = message.data_objects[0];
    if ((first & 0xC0000000) != 0 || ((first >> 10) & 0x3FF) != 100) {
      logger_.LogMessage(Logger::LogLevel::kWarning, __FILE__, __LINE__,
          "PD source capability 1 is not a fixed 5 V supply\n");
      return false;
    }
    capabilities_ = message;
    return RequestPower(now_ms);
  }
  if (control && type == 3 && status_.state == State::kWaitingAccept) {
    status_.state = State::kWaitingPowerReady;
    deadline_ms_ = now_ms + 550;
    return true;
  }
  if (control && type == 6 && status_.state == State::kWaitingPowerReady) {
    const bool capture = !status_.first_ps_ready_seen;
    if (capture) {
      status_.first_ps_ready_seen = true;
      status_.first_ps_ready_pdo = status_.requested_pdo;
      status_.first_ps_ready_target_mv = requested_voltage_mv_;
      status_.first_ps_ready_pmic_valid =
          chip_.GetVbusVoltage(status_.first_ps_ready_pmic_mv);
    }
    // 官方 TCPC 驱动用 POWER_STATUS 判定 VBUS，不在收到 PS_RDY 的
    // 同一次中断里用电压 ADC 判定失败。先保持保守限流，等待 PMIC ADC 稳定。
    if (capture) {
      chip_.GetTcpcVbusVoltage(status_.first_ps_ready_tcpc_mv);
      status_.first_ps_ready_result = PsReadyResult::kVoltageSettling;
    }
    status_.state = State::kWaitingVoltageStable;
    deadline_ms_ = now_ms + kVbusSettleTimeoutMs;
    return true;
  }
  if (control && type == 6 && status_.state == State::kWaitingVoltageStable) {
    return true;  // 重发的 PS_RDY 不重置稳定等待超时。
  }
  if (control && (type == 4 || type == 12 || type == 16) &&
      status_.state == State::kWaitingAccept) {
    if (requested_pps_ && !pps_rejected_) {
      pps_rejected_ = true;
      return RequestPower(now_ms);
    }
    return false;
  }
  if (!control && type == 15) {
    // 不支持替代模式；结构化 VDM 请求返回 NAK，其余 VDM 不响应。
    const uint32_t vdm = message.data_objects[0];
    if ((vdm & 0x80C0) == 0x8000) {
      const uint32_t nak = vdm | 0x80;
      return Send(15, now_ms, &nak);
    }
    return true;
  }
  if (control && type == 8) {
    // Sink_Capabilities 与板级输入额定值一致，不宣称不支持的电压。
    std::array<uint32_t, 5> sink_capabilities{};
    uint8_t count = 0;
    const uint32_t fixed_current = config_.max_input_current_ma / 10;
    constexpr uint16_t kFixedVoltages[] = {5000, 9000, 12000, 15000};
    for (const uint16_t voltage : kFixedVoltages) {
      if (voltage <= config_.max_input_voltage_mv) {
        sink_capabilities[count++] =
            ((static_cast<uint32_t>(voltage) / 50) << 10) | fixed_current;
      }
    }
    if (config_.enable_pps &&
        revision_ == Axp517::PdRevision::kRev30) {
      sink_capabilities[count++] = 0xC0000000u |
          ((static_cast<uint32_t>(config_.max_input_voltage_mv / 100)) << 17) |
          (50u << 8) | (config_.max_input_current_ma / 50);
    }
    return Send(4, now_ms, sink_capabilities.data(), count);
  }
  if (control && (type == 9 || type == 10 || type == 11)) {
    return Send(4, now_ms);  // Reject，不接受任何角色切换。
  }
  // GoodCRC 由控制器处理。
  if (control && (type == 1 || type == 3 || type == 4 || type == 6 ||
                     type == 12 || type == 16))
    return true;
  return Send(revision_ == Axp517::PdRevision::kRev30 ? 16 : 4, now_ms);
}

bool Axp517Sink::Poll(uint64_t now_ms, bool enabled) {
  return Poll(now_ms, enabled, config_.contract_charge_current_ma);
}

bool Axp517Sink::Poll(
    uint64_t now_ms, bool enabled, uint16_t charge_current_ma) {
  if (!config_valid_ ||
      (config_.manage_charge_current && charge_current_ma > 5120)) {
    status_.state = State::kError;
    return false;
  }
  const bool changed = config_.manage_charge_current &&
      contract_charge_current_ma_ != charge_current_ma;
  const bool was_error = status_.state == State::kError;
  // 未建立合同（含错误状态）时，电池切换仍更新充电电流，但不超过
  // 应用配置的回退上限；已有合同的升流留到 Process 确认状态后。
  if (config_.manage_charge_current &&
      (!charge_current_initialized_ ||
       (changed && (status_.state != State::kReady ||
                    charge_current_ma < status_.charge_current_ma)))) {
    const uint16_t safe_ma =
        charge_current_initialized_ && status_.state == State::kReady
        ? charge_current_ma
        : std::min(config_.fallback_charge_current_ma, charge_current_ma);
    if (!chip_.SetChargeCurrent(safe_ma)) goto fail;
    status_.charge_current_ma = safe_ma;
    charge_current_initialized_ = true;
  }
  contract_charge_current_ma_ = charge_current_ma;
  if (!Process(now_ms, enabled)) {
    // 已安全停机的错误状态仅监测拔插，不能每次轮询都重写限流寄存器。
    if (was_error && status_.state == State::kError && !active_) return false;
    goto fail;
  }
  if (changed && config_.manage_charge_current &&
      status_.state == State::kReady && status_.battery_present &&
      status_.attached && status_.charge_current_ma != charge_current_ma) {
    if (!chip_.SetChargeCurrent(charge_current_ma)) goto fail;
    status_.charge_current_ma = charge_current_ma;
  }
  return true;
fail:
  // 所有失败出口都先限流，不允许清中断等辅助操作失败后留下高电流。
  if (status_.state != State::kError) status_.failure_stage = status_.state;
  LimitCharging();
  if (active_) {
    chip_.TransmitPdMessage(Axp517::PdTransmitType::kHardReset, nullptr);
    chip_.SetPdReceiveMask(0);
    active_ = false;
  }
  status_.state = State::kError;
  return false;
}

bool Axp517Sink::Restart(uint64_t now_ms) {
  if (status_.state != State::kError) return false;
  // 复用禁用路径撤销合同并清除本次选择；下一次 Poll 再开始新协商。
  return Poll(now_ms, false, contract_charge_current_ma_) &&
         status_.state == State::kDisabled;
}

bool Axp517Sink::Process(uint64_t now_ms, bool enabled) {
  Axp517::Status power;
  Axp517::CcStatus cc;
  if (!chip_.GetStatus(power) || !chip_.GetCcStatus(cc)) {
    if (status_.state != State::kError) LimitCharging();
    if (active_) {
      chip_.TransmitPdMessage(Axp517::PdTransmitType::kHardReset, nullptr);
      chip_.SetPdReceiveMask(0);
      active_ = false;
    }
    status_.enabled = enabled;
    status_.battery_present = false;
    status_.state = State::kError;
    return false;
  }
  status_.enabled = enabled;
  status_.battery_present = power.battery_present;
  // AXP517 在极性尚未固定时可能同时报告 CC1/CC2 为 Rp。此时虽然
  // GetCcStatus() 会将其标记为 Debug Accessory，但它仍然是有效的 Sink
  // 接入；必须先执行 SetPolarity()，才能关闭非活动 CC 并启动 PD 接收。
  status_.attached = cc.sink_attached;
  const bool allowed = enabled && status_.attached &&
      (!config_.require_battery_present || power.battery_present);
  if (!allowed) {
    selected_ = false;
    bool ready = true;
    if (active_ || status_.state != State::kDisabled) {
      const bool limited = LimitCharging();
      // 板级策略禁用 PD 时立即限流，并要求电源退出高压合同。
      const bool reset_ok =
          !active_ || !status_.attached ||
          chip_.TransmitPdMessage(Axp517::PdTransmitType::kHardReset, nullptr);
      const bool stopped = chip_.SetPdReceiveMask(0);
      active_ = false;
      tx_pending_ = false;
      status_.state =
          limited && stopped && reset_ok ? State::kDisabled : State::kError;
      ready = limited && stopped && reset_ok;
    }
    // SetPolarity() 会将非活动 CC 置为 Open。拔插或协议失败后必须恢复
    // Rd/Rd 并重新执行 Look4Connection。C-to-C 电源在看到 Rd 之前不会
    // 提供 VBUS，因此不能以 vbus_good 作为重新配置 CC 的前提。
    // 仅在未接入时重试，并限制频率，避免每 2 ms 重写角色控制寄存器。
    if (!cc.sink_attached && now_ms >= attach_retry_ms_) {
      const bool armed =
          chip_.SetTypeCRole(Axp517::TypeCRole::kSink);
      attach_retry_ms_ = now_ms + 250;
      if (!armed) status_.state = State::kError;
      ready = ready && armed;
    }
    return ready;
  }
  attach_retry_ms_ = 0;
  if (!selected_) {
    selected_ = true;
    selected_since_ms_ = now_ms;
    status_.failure_stage = State::kDisabled;
    status_.last_pd_alerts = 0;
    status_.rx_count = 0;
    status_.source_caps_count = 0;
    status_.request_count = 0;
    status_.accept_count = 0;
    status_.ps_ready_count = 0;
    status_.tx_failed_count = 0;
    status_.fault_count = 0;
    status_.last_fault_status = 0;
    status_.requested_pdo = 0;
    status_.requested_voltage_mv = 0;
    status_.requested_current_ma = 0;
    status_.requested_pps = false;
    status_.first_ps_ready_seen = false;
    status_.first_ps_ready_pdo = 0;
    status_.first_ps_ready_target_mv = 0;
    status_.first_ps_ready_tcpc_mv = 0;
    status_.first_ps_ready_pmic_mv = 0;
    status_.first_ps_ready_pmic_valid = false;
    status_.first_ps_ready_result = PsReadyResult::kNotSeen;
    status_.settled_pmic_mv = 0;
    status_.settled_pmic_valid = false;
    status_.settled_tcpc_mv = 0;
    status_.settled_tcpc_valid = false;
    pps_rejected_ = false;
    if (!LimitCharging()) {
      status_.state = State::kError;
      return false;
    }
  }
  if (now_ms - selected_since_ms_ < config_.enable_debounce_ms) return true;
  if (status_.state == State::kError) return false;
  if (!active_) {
    message_id_ = 0;
    received_id_ = -1;
    // AXP517 官方驱动在合同建立前以 PD 2.0 初始化消息头；收到首个
    // Source_Capabilities 后再跟随对端报文中的协议版本。
    revision_ = Axp517::PdRevision::kRev20;
    queried_capabilities_ = false;
    const bool cc1_rp = IsSinkRp(cc.cc1);
    const bool cc2_rp = IsSinkRp(cc.cc2);
    const Axp517::Polarity polarity =
        !cc1_rp && cc2_rp ? Axp517::Polarity::kCc2
                          : Axp517::Polarity::kCc1;
    if (!chip_.SetPolarity(polarity)) {
      status_.state = State::kError;
      return false;
    }
    Axp517::CcStatus oriented_cc;
    if (!chip_.GetCcStatus(oriented_cc) ||
        !(polarity == Axp517::Polarity::kCc1
                ? IsSinkRp(oriented_cc.cc1)
                : IsSinkRp(oriented_cc.cc2))) {
      logger_.LogMessage(Logger::LogLevel::kWarning, __FILE__, __LINE__,
          "PD polarity selection did not retain Rp on the active CC\n");
      status_.state = State::kError;
      return false;
    }
    status_.attached = true;
    if (!chip_.SetPdMessageHeader(false, false, revision_) ||
        !chip_.ClearPdAlerts(0xEFFF) || !chip_.SetVbusPath(false, true) ||
        // 对齐官方 Sink 策略：建立合同期间只接收 SOP 消息。
        !chip_.SetPdReceiveMask(0x01)) {
      status_.state = State::kError;
      return false;
    }
    active_ = true;
    status_.state = State::kWaitingCapabilities;
    deadline_ms_ = now_ms + 500;
  }
  uint16_t alerts = 0;
  if (!chip_.GetPdAlerts(alerts)) return Fail(true);
  status_.last_pd_alerts = alerts;
  if ((alerts & 0x0200) != 0) {
    Axp517::TcpcStatus tcpc;
    if (!chip_.GetTcpcStatus(tcpc) || !chip_.ClearTcpcFaults(tcpc.fault) ||
        !chip_.ClearPdAlerts(0x0200))
      return Fail(true);
    ++status_.fault_count;
    status_.last_fault_status = tcpc.fault;
    // AXP517 可能在收到 Source_Capabilities 的同一批告警中报告 FAULT。
    // 官方驱动仅清除故障并继续处理 RX；在此复位会丢掉已收到的能力消息。
    if (tcpc.fault != 0) {
      logger_.LogMessage(Logger::LogLevel::kWarning, __FILE__, __LINE__,
          "PD TCPC fault cleared (status: 0x%02X, alerts: 0x%04X)\n",
          tcpc.fault, alerts);
    }
  }
  if ((alerts & 0x0408) != 0) {
    if (!chip_.ClearPdAlerts(alerts & 0x040C)) return false;
    return Fail((alerts & 8) == 0);
  }
  if ((alerts & kTxAlerts) != 0) {
    if (!chip_.ClearPdAlerts(alerts & kTxAlerts)) return false;
    if ((alerts & 0x0010) != 0) ++status_.tx_failed_count;
    if (tx_pending_) {
      tx_pending_ = false;
      if ((alerts & 0x0040) == 0) return Fail(true);
      message_id_ = (message_id_ + 1) & 7;
      if (status_.state == State::kWaitingAccept) {
        // SenderResponse 从发送完成开始计时。
        deadline_ms_ = now_ms + 30;
      }
    }
  }
  if (tx_pending_ && now_ms >= tx_deadline_ms_) return Fail(true);
  if ((alerts & 4) != 0) {
    Axp517::PdMessage message;
    if (!chip_.ReceivePdMessage(message) || !Receive(message, now_ms)) {
      return Fail(true);
    }
  }
  if (status_.state == State::kWaitingVoltageStable) {
    uint16_t pmic_mv = 0;
    const bool pmic_valid = chip_.GetVbusVoltage(pmic_mv);
    if (pmic_valid &&
        pmic_mv >= requested_voltage_mv_ * 9 / 10 &&
        pmic_mv <= requested_voltage_mv_ * 11 / 10) {
      status_.settled_pmic_mv = pmic_mv;
      status_.settled_pmic_valid = true;
      status_.settled_tcpc_valid =
          chip_.GetTcpcVbusVoltage(status_.settled_tcpc_mv);
      // 仅在 PMIC 输入端实际测到目标电压后提升输入及充电电流。
      if (!chip_.ApplyNegotiatedInputVoltage(requested_voltage_mv_, 4400)) {
        status_.first_ps_ready_result = PsReadyResult::kInputVoltageConfigFailed;
        return Fail(true);
      }
      if (!chip_.SetInputCurrentLimit(requested_current_ma_)) {
        status_.first_ps_ready_result = PsReadyResult::kInputCurrentConfigFailed;
        return Fail(true);
      }
      if (config_.manage_charge_current &&
          !chip_.SetChargeCurrent(contract_charge_current_ma_)) {
        status_.first_ps_ready_result = PsReadyResult::kChargeCurrentConfigFailed;
        return Fail(true);
      }
      status_.first_ps_ready_result = PsReadyResult::kReady;
      status_.state = State::kReady;
      status_.pps = requested_pps_;
      status_.voltage_mv = requested_voltage_mv_;
      status_.current_ma = requested_current_ma_;
      status_.charge_current_ma = config_.manage_charge_current
                                      ? contract_charge_current_ma_ : 0;
      refresh_ms_ = now_ms + kPpsRefreshMs;
      return true;
    }
    if (now_ms >= deadline_ms_) {
      status_.settled_pmic_mv = pmic_mv;
      status_.settled_pmic_valid = pmic_valid;
      status_.settled_tcpc_valid =
          chip_.GetTcpcVbusVoltage(status_.settled_tcpc_mv);
      status_.first_ps_ready_result = PsReadyResult::kVoltageMismatch;
      logger_.LogMessage(Logger::LogLevel::kWarning, __FILE__, __LINE__,
          "PD PMIC VBUS mismatch after settling (measured: %u mV, valid: %d, requested: %u mV)\n",
          pmic_mv, pmic_valid, requested_voltage_mv_);
      return Fail(true);
    }
    return true;
  }
  if (status_.state == State::kReady) {
    // PD 3.0 的 Sink 主动发起 AMS 必须等待电源以 Rp=3 A 表示 SinkTxOK。
    const bool sink_tx_ok = cc.cc1 == Axp517::CcState::kRp3000Ma ||
                            cc.cc2 == Axp517::CcState::kRp3000Ma;
    if (status_.pps && now_ms >= refresh_ms_ + 3000) {
      return Fail(true);
    }
    if (status_.pps && now_ms >= refresh_ms_ && !tx_pending_ && sink_tx_ok) {
      if (!Send(2, now_ms, &request_)) return Fail(true);
      status_.state = State::kWaitingAccept;
      deadline_ms_ = now_ms + 100;
    }
    return true;
  }
  if (now_ms >= deadline_ms_) {
    if (status_.state == State::kWaitingCapabilities &&
        !queried_capabilities_) {
      queried_capabilities_ = true;
      if (!Send(7, now_ms)) return Fail(true);
      deadline_ms_ = now_ms + 500;
    } else {
      return Fail(true);
    }
  }
  return true;
}

}  // namespace cpp_bus_driver
