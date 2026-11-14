/*
 * @Description: AW933xx 电容检测、佩戴及滑动识别驱动
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {

class Aw933xx final : public I2cChipBase {
 public:
  enum class OperatingMode : uint8_t {
    kActive = 1,
    kSleep = 2,
    kDeepSleep = 3,
  };

  enum class InterruptPin : uint8_t {
    kDisabled,
    kCs2,
    kCs5,
  };

  enum class ElectrodeMode : uint8_t {
    kHighImpedance = 0,
    kMeasure = 1,
    kShield = 2,
    kGround = 3
  };

  enum class Widget : uint8_t {
    kDisabled = 0,
    kButton,
    kSlider1d,
    kSlider2d,
    kCircularSlider,
    kWear,
    kForce
  };

  static constexpr uint8_t kChannelCount = 12;
  static constexpr uint8_t kSliderCount = 10;
  static constexpr uint8_t kWearCount = 2;

  struct ChannelData {
    // 保留芯片 Q10 原始值，显示时除以 1024；不要将有符号差值转成无符号。
    int32_t raw = 0;
    int32_t valid = 0;
    int32_t baseline = 0;
    int32_t difference = 0;
  };

  struct WidgetConfig {
    Widget widget = Widget::kSlider1d;  // 按键、滑条、佩戴或压力识别算法类型。
    uint8_t first_channel = 0;          // 算法使用的起始逻辑 CH，范围 0~11。
    uint8_t x_channels = 3;             // X 轴连续通道数，范围 0~12。
    uint8_t y_channels = 0;             // Y 轴连续通道数，范围 0~12。
    uint8_t debounce = 3;               // 芯片触摸消抖计数。
    uint16_t scan_period_ms = 20;       // 算法扫描周期，范围 10~100 ms。
    uint32_t touch_threshold = 50000;   // 硬件触摸阈值，使用芯片原始单位。
    uint32_t release_threshold = 0;     // 硬件释放阈值，使用芯片原始单位。
    uint32_t position_threshold = 50000;  // 位置计算阈值，芯片原始单位。
    uint16_t click_interval_ms = 500;     // 连击间隔阈值，单位 ms。
    uint16_t long_press_ms = 2000;        // 长按阈值，单位 ms。
    uint16_t longer_press_ms = 3000;      // 超长按阈值，单位 ms。
    uint16_t super_long_press_ms = 4000;  // 特长按阈值，单位 ms。
    uint16_t minimum_distance = 15;       // 滑动方向事件的最小距离阈值。
    bool interrupt_enabled = true;        // 是否启用该算法组的触摸中断。
  };

  struct SliderState {
    uint32_t button_state = 0;
    uint16_t position_x = 0;
    uint16_t position_y = 0;
    int16_t distance_x = 0;
    int16_t distance_y = 0;
  };

  struct SliderEvent {
    uint8_t press = 0;  // 1：按下；3：长按；7：超长按。
    uint8_t clicks = 0;
    uint8_t directions = 0;   // 位 0/1/2/3：上/下/左/右。
    uint8_t electrode_x = 0;  // 芯片报告的序号，0 表示无有效位置。
    uint8_t electrode_y = 0;
  };

  struct Events {
    uint32_t source = 0;
    uint16_t proximity0 = 0;  // 阈值 0 的当前通道状态，bit n 对应 CHn。
    uint16_t proximity1 = 0;  // 阈值 1 的当前通道状态，bit n 对应 CHn。
    uint8_t wear = 0;
    uint8_t learning_finished = 0;
    std::array<SliderEvent, kSliderCount> sliders{};
  };

  // 可由上层存入 NVS/Flash；驱动不绑定平台存储，也不读取未初始化的 Flash。
  struct WearCalibration {
    std::array<uint32_t, 4> values{};
  };

  struct ChannelStatistics {
    int32_t minimum = 0;
    int32_t maximum = 0;
    int32_t average = 0;
    int64_t peak_to_peak = 0;
  };

  explicit Aw933xx(
      std::shared_ptr<I2cBusBase> bus, int16_t address = kDefaultI2cAddress)
      : I2cChipBase(bus, address) {}

  /**
   * @brief 识别型号、复位、加载官方固件和基础参数；成功后停在睡眠模式。
   * @param freq_hz I2C 时钟频率，范围 1~400000 Hz。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool Init(int32_t freq_hz = kDefaultI2cFrequencyHz) override;

  /**
   * @brief 停止芯片并释放总线。
   * @param delete_bus true 表示同时释放总线资源。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 软件复位并等待初始化完成，复位后需重新配置采样和算法。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool SoftwareReset();

  /**
   * @brief 读取芯片标识。
   * @param value 读取结果输出指针，失败时保持原值。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool GetChipId(uint32_t* value);

  /**
   * @brief 读取当前固件版本。
   * @param value 读取结果输出指针，失败时保持原值。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool GetFirmwareVersion(uint32_t* value);

  /**
   * @brief 获取当前型号可用的物理 CS 位图。
   * @return 可用的物理 CS 位图，尚未识别型号时为 0。
   */
  uint16_t electrode_mask() const { return electrode_mask_; }

  /**
   * @brief 切换工作模式，从深睡眠唤醒时先恢复振荡器。
   * @param mode 目标工作模式。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool SetOperatingMode(OperatingMode mode);

  /**
   * @brief 在睡眠状态配置扫描通道和扫描周期。
   * @param channels 12 位逻辑通道位图。
   * @param period_ms 扫描周期，范围 10~100 ms。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ConfigureScan(uint16_t channels, uint16_t period_ms = 20);

  /**
   * @brief 在睡眠状态为逻辑通道映射测量、屏蔽、接地及高阻电极。
   * @param channel 逻辑通道，范围 0~11。
   * @param electrodes 物理 CS0~CS11 模式；不存在及中断复用脚须为高阻。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ConfigureElectrodes(uint8_t channel,
      const std::array<ElectrodeMode, kChannelCount>& electrodes);

  /**
   * @brief 在睡眠状态配置硬件交互算法组，类型由 config.widget 指定。
   * @param widget_index 硬件算法组索引，范围 0~9。
   * @param config 算法组参数，first_channel 为逻辑 CH 起点。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ConfigureWidget(uint8_t widget_index, const WidgetConfig& config);

  /**
   * @brief 在睡眠状态配置检测与参考两个逻辑通道的差分佩戴算法。
   * @param wear 佩戴组，范围 0~1。
   * @param detect_channel 佩戴检测逻辑通道。
   * @param reference_channel 佩戴参考逻辑通道，须与检测通道不同。
   * @param on_threshold 戴上阈值，保持芯片原始单位。
   * @param off_threshold 摘下阈值，保持芯片原始单位。
   * @param enabled 是否启用该佩戴组。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ConfigureWear(uint8_t wear, uint8_t detect_channel,
      uint8_t reference_channel, uint32_t on_threshold, uint32_t off_threshold,
      bool enabled = true);

  /**
   * @brief 配置 AW93307 或 AW93305 BFOR 的 CS2/CS5 中断复用。
   * @param pin 关闭中断复用或选择 CS2/CS5；必须先关闭扫描再配置。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool SetInterruptPin(InterruptPin pin);

  /**
   * @brief 配置硬件中断使能位图。
   * @param mask 中断使能位图，位 0~19 有效，置 1 使能对应中断。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool SetInterruptMask(uint32_t mask);

  /**
   * @brief 触发指定扫描通道的寄生电容校准，不自动等待。
   * @param channels 12 位逻辑通道位图。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool Calibrate(uint16_t channels);

  /**
   * @brief 读取一个逻辑通道的原始值、有效值、基线及有符号差值。
   * @param channel 逻辑通道，范围 0~11。
   * @param data 输出完整通道采样，任一读取失败时保持原值。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ReadChannel(uint8_t channel, ChannelData* data);

  /**
   * @brief 读取 AFE 补偿电容，支持无符号、有符号及互电容模式。
   * @param channel 逻辑通道，范围 0~11。
   * @param picofarads 输出补偿电容值，单位 pF。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ReadOffsetCapacitance(uint8_t channel, float* picofarads);

  /**
   * @brief 读取硬件滑动状态、当前位置和移动距离。
   * @param slider 硬件滑动组，范围 0~9。
   * @param state 输出滑动状态，读取失败时保持原值。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ReadSliderState(uint8_t slider, SliderState* state);

  /**
   * @brief 读取两组佩戴状态位图。
   * @param state 输出两组佩戴位图，位 0/1 分别对应佩戴组 0/1。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ReadWearState(uint8_t* state);

  /**
   * @brief 读清中断源并解析所有置位的滑动事件，限任务上下文调用。
   * @param events 输出事件和当前通道状态，失败时保持原值；source 已被读清。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   * @note 通道状态不会自动去重，状态变化上报由调用方比较前后位图实现。
   */
  bool ReadEvents(Events* events);

  /**
   * @brief 发送佩戴自动学习命令。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool StartWearLearning();

  /**
   * @brief 读取两组学习数据并确认完成，由调用方决定是否持久化。
   * @param data 输出两组佩戴校准数据，读取或确认失败时保持原值。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool FinishWearLearning(std::array<WearCalibration, kWearCount>* data);

  /**
   * @brief 读取指定佩戴组的四个校准寄存器。
   * @param wear 佩戴组，范围 0~1。
   * @param data 输出指定佩戴组的校准数据，读取失败时保持原值。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ReadWearCalibration(uint8_t wear, WearCalibration* data);

  /**
   * @brief 写入已由调用方验证的佩戴校准数据。
   * @param wear 佩戴组，范围 0~1。
   * @param data 需要恢复的佩戴校准数据。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool RestoreWearCalibration(uint8_t wear, const WearCalibration& data);

  /**
   * @brief 暂停佩戴算法并采集空载基准，无论成功与否都尝试恢复算法。
   * @param wear 佩戴组，范围 0~1。
   * @param data 输出空载校准数据，采集或恢复算法失败时保持原值。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool CaptureWearCalibration(uint8_t wear, WearCalibration* data);

  /**
   * @brief 采集差值均值及峰峰值，用于空载噪声及接近信号测试。
   * @param channel 逻辑通道，范围 0~11。
   * @param samples 采样次数，范围 2~1000。
   * @param interval_ms 采样间隔，范围 10~1000 ms。
   * @param statistics 输出 Q10 单位统计值，失败时保持不变。
   * @return 全部采样成功返回 true，否则返回 false。
   */
  bool MeasureDifference(uint8_t channel, uint16_t samples,
      uint16_t interval_ms, ChannelStatistics* statistics);

  /**
   * @brief 暂停采样检测 CS 对地及电源短路，并恢复 GPIO 和工作模式。
   * @param ground 输出疑似对地短路的物理 CS 位图。
   * @param supply 输出疑似对电源短路的物理 CS 位图。
   * @return 检测及恢复成功返回 true，短路结果由位图返回。
   * @note 测试期间需保持电极空载；不能在 ISR 中调用。
   */
  bool CheckElectrodeShorts(uint16_t* ground, uint16_t* supply);

 private:
  enum class Register : uint16_t {
    kScanEnable = 0x0000,
    kCalibration = 0x0004,
    kScanControl2 = 0x0008,
    kScanPeriod = 0x000C,
    kProximity0 = 0x0020,
    kProximity1 = 0x0024,
    kFirmwareVersion = 0x003C,
    kAfeConfig0 = 0x0084,
    kAfeOffset = 0x0088,
    kElectrodeSelect0 = 0x0098,
    kElectrodeSelect1 = 0x009C,
    kValid = 0x00D8,
    kBaseline = 0x00DC,
    kDifference = 0x00E0,
    kRaw = 0x00F0,
    kWearControl = 0x091C,
    kWearState = 0x0920,
    kWearOnThreshold = 0x0924,
    kWearOffThreshold = 0x0928,
    kWearMode = 0x09A0,
    kLearnedOffset = 0x0A14,
    kSliderTouchThreshold = 0x0AB4,
    kSliderControl = 0x0AC4,
    kSliderState = 0x0AD4,
    kSliderPositionThreshold = 0x0AE4,
    kSliderConfig = 0x0AEC,
    kSliderDistanceThreshold = 0x0AF4,
    kSliderPosition = 0x0AFC,
    kSliderDistance = 0x0B04,
    kSliderReport = 0x0B08,
    kPinMux = 0xC100,
    kRamPower = 0xC400,
    kGpioDirection = 0xE004,
    kInterruptOutput = 0xE018,
    kCommand = 0xF008,
    kInterruptSource = 0xF080,
    kInterruptEnable = 0xF084,
    kCpuControl = 0xF800,
    kClockControl = 0xFF00,
    kReset = 0xFF0C,
    kChipId = 0xFF10,
    kRamAccess = 0xFFE4,
    kPeripheralAccess = 0xFFF4,
  };

  enum class Command : uint32_t {
    kActive = 0x01,
    kSleep = 0x02,
    kDeepSleep = 0x03,
    kStartWearLearning = 0x0A,
    kFinishWearLearning = 0x0C,
  };

  static constexpr int16_t kDefaultI2cAddress = 0x12;
  static constexpr int32_t kDefaultI2cFrequencyHz = 400000;

  static constexpr uint32_t kInitializationTimeoutMs = 10000;
  static constexpr uint32_t kInitializationPollIntervalMs = 10;

  static constexpr uint16_t kChannelStride = 0xB8;
  static constexpr uint16_t kSliderStride = 0x5C;
  static constexpr uint16_t kWearStride = 0x84;

  /**
   * @brief 写完内置官方 V9.0.4 固件的两段数据，再分包回读校验。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool LoadFirmware();

  /**
   * @brief 校验官方固件容器，写完两段后回读比对，全部一致才启动 CPU。
   * @param image AWINIC V2 固件容器起始地址。
   * @param size 固件容器完整字节数，包含 80 字节头。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool LoadFirmware(const uint8_t* image, size_t size);

  /**
   * @brief 依次写入寄存器参数表，首个写入失败即停止。
   * @param settings 每项依次为 16 位寄存器地址和 32 位数据。
   * @param count 参数表项数。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ApplySettings(const std::array<uint32_t, 2>* settings, size_t count);

  /**
   * @brief 按 16 位地址读取 32 位大端数据，失败时不修改输出。
   * @param address 四字节对齐的 16 位寄存器地址。
   * @param value 读取结果输出指针，失败时保持原值。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool ReadRegister(uint16_t address, uint32_t* value);

  /**
   * @brief 按 16 位地址写入 32 位大端数据。
   * @param address 四字节对齐的 16 位寄存器地址。
   * @param value 待写入的寄存器数据。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool WriteRegister(uint16_t address, uint32_t value);

  /**
   * @brief 更新 mask 中为 1 的位，读失败时不继续写入。
   * @param address 四字节对齐的 16 位寄存器地址。
   * @param mask 需要更新的位掩码，置 1 的位参与修改。
   * @param value 待写入的寄存器数据。
   * @return 操作成功返回 true，参数无效或通信失败返回 false。
   */
  bool UpdateRegister(uint16_t address, uint32_t mask, uint32_t value);

  /**
   * @brief 轮询初始化中断，轮询延时累计上限为 10 秒，不含 I2C 事务耗时。
   * @return 初始化完成返回 true，通信失败或超时返回 false。
   */
  bool WaitForInitialization();

  /**
   * @brief 分包写入固件区域，可将剩余空间填充为 0xFF。
   * @param address 固件区域起始地址。
   * @param data 固件数据起始指针。
   * @param size 有效固件字节数。
   * @param capacity 区域容量，不能小于有效固件大小。
   * @param fill 是否填充区域剩余空间。
   * @return 全部数据写入成功返回 true，否则返回 false。
   */
  bool TransferFirmwareRegion(uint16_t address, const uint8_t* data,
      size_t size, size_t capacity, bool fill);

  /**
   * @brief 分包回读固件区域，与目标数据及可选的 0xFF 填充区比较。
   * @param address 固件区域起始地址。
   * @param data 期望的固件数据指针。
   * @param size 有效固件字节数。
   * @param capacity 区域容量，不能小于有效固件大小。
   * @param fill 是否同时校验区域剩余空间为 0xFF。
   * @return 数据全部一致返回 true，读取失败或内容不一致返回 false。
   */
  bool VerifyFirmwareRegion(uint16_t address, const uint8_t* data, size_t size,
      size_t capacity, bool fill);

  /**
   * @brief 连续读取寄存器或固件数据，并记录访问失败信息。
   * @param address 16 位起始地址。
   * @param data 接收缓冲区。
   * @param size 读取字节数，范围 1~128，不能超出地址空间。
   * @return 读取成功返回 true，否则返回 false；不自动重试读清寄存器。
   */
  bool ReadRegister(uint16_t address, uint8_t* data, size_t size);

  /**
   * @brief 连续写入寄存器或固件数据，并记录访问失败信息。
   * @param address 16 位起始地址。
   * @param data 待写入的数据缓冲区。
   * @param size 写入字节数，范围 1~128，不能超出地址空间。
   * @return 写入成功返回 true，否则返回 false。
   */
  bool WriteRegister(uint16_t address, const uint8_t* data, size_t size);

  /**
   * @brief 检查逻辑通道编号是否在芯片支持的范围内。
   * @param channel 待检查的逻辑通道编号。
   * @return 编号为 0~11 返回 true，否则返回 false。
   */
  bool CheckChannel(uint8_t channel) const;

  OperatingMode operating_mode_ = OperatingMode::kActive;
  uint16_t electrode_mask_ = 0;
  InterruptPin interrupt_pin_ = InterruptPin::kDisabled;
};

}  // namespace cpp_bus_driver
