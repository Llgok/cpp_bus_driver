/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:42:47
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Aw862xx final : public ChipI2cGuide {
 public:
  enum class PlayMode {
    kRam = 0,
    kRtp,
    kCont,
    kNoPlay,
  };

  enum class GlobalStatus {
    kFalse = -1,

    kStandby = 0,
    kCont = 6,
    kRam,
    kRtp,
    kTrig,
    kBrake = 11,
  };

  enum class SampleRate {
    kRate24Khz = 0,
    kRate48Khz,
    kRate12Khz,
  };

  enum class D2sGain {
    kGain1 = 0,
    kGain2,
    kGain4,
    kGain5,
    kGain8,
    kGain10,
    kGain20,
    kGain40,
  };

  enum class ForceMode {
    kActive,   // 激活模式
    kStandby,  // 待机模式
  };

  struct SystemStatus {
    bool uvls_flag = false;              // VDD电压低于UV电压标志
    bool rtp_fifo_empty = true;          // RTP模式FIFO为空标志
    bool rtp_fifo_full = false;          // RTP模式FIFO为满标志
    bool over_current_flag = false;      // 过流标志
    bool over_temperature_flag = false;  // 过温标志
    bool playback_flag = false;          // 回放标志
  };

#include "aw862xx_haptic_waveform_table.h"

  explicit Aw862xx(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;

  uint8_t GetDeviceId();

  /**
   * @brief 软件复位
   * @return
   * @Date 2025-03-07 17:31:30
   */
  bool SoftwareReset();

  /**
   * @brief 获取输入电压
   * @return
   * @Date 2025-03-07 17:31:39
   */
  float GetInputVoltage();

  /**
   * @brief 设置GO TRIG的波形播放模式，kRam/kRtp/kCont/NO kPlay 模式
   * @param mode 使用Play_Mode::配置
   * @return
   * @Date 2025-02-28 10:37:12
   */
  bool SetPlayMode(PlayMode mode);

  /**
   * @brief kRam/kRtp/kCont 模式播放触发位，当设置为 1
   * 时，芯片将播放其中一种播放模式
   * @return
   * @Date 2025-02-28 10:41:41
   */
  bool SetGoFlag();

  /**
   * @brief 获取全局状态
   * @return
   * @Date 2025-02-28 10:47:13
   */
  GlobalStatus GetGlobalStatus();

  /**
   * @brief RTP模式播放waveform库文件
   * @param *waveform_data 波形数据指针
   * @param length 波形输出长度
   * @return
   * @Date 2025-03-07 17:31:52
   */
  bool RunRtpPlaybackWaveform(const uint8_t* waveform_data, size_t length);

  /**
   * @brief 设置播放waveform库文件的采样率
   * @param rate
   * @return
   * @Date 2025-03-07 17:50:35
   */
  bool SetWaveformDataSampleRate(SampleRate rate);

  /**
   * @brief 设置在播放的时候改变增益
   * @param enable [true]：在播放的时候改变增益，[false]：在播放的时候不改变增益
   * @return
   * @Date 2025-03-07 17:57:48
   */
  bool SetPlayingChangedGainBypass(bool enable);

  /**
   * @brief 设置D2S增益
   * @param gain 使用D2s_Gain::配置
   * @return
   * @Date 2025-03-07 18:05:22
   */
  bool SetD2sGain(D2sGain gain);

  /**
   * @brief 调整LRA（线性振动马达）的频率，以适应LRA共振频率的偏差
   * @param freq_hz 值范围：0~63
   * @return
   * @Date 2025-03-10 09:33:31
   */
  bool SetLraOscFrequency(uint8_t freq_hz);

  /**
   * @brief 设置是否启动f0校验模式
   * @param enable [true]：启动，[false]：关闭
   * @return
   * @Date 2025-03-10 09:27:41
   */
  bool SetF0DetectionMode(bool enable);

  /**
   * @brief 设置振动马达开关
   * @param enable [true]：启动，[false]：关闭
   * @return
   * @Date 2025-03-10 09:40:13
   */
  bool SetTrackSwitch(bool enable);

  /**
   * @brief 设置在RTP/kRam/CONT播放模式停止后启用自动制动（当设置为1时）
   * @param enable [true]：启动，[false]：关闭
   * @return
   * @Date 2025-03-10 09:45:50
   */
  bool SetAutoBrakeStop(bool enable);

  /**
   * @brief
   * 用于控制第1个连续驱动器的输出电压级别，它通过设置寄存器的值来调节输出电压
   * @param level 值范围：0~127
   * @return
   * @Date 2025-03-10 09:52:04
   */
  bool SetContDrive1Level(uint8_t level);

  /**
   * @brief
   * 用于控制第2个连续驱动器的输出电压级别，它通过设置寄存器的值来调节输出电压
   * @param level 值范围：0~127
   * @return
   * @Date 2025-03-10 09:52:04
   */
  bool SetContDrive2Level(uint8_t level);

  /**
   * @brief 设置第1次连续驱动的半循环次数
   * @param times 值范围：0~255
   * @return
   * @Date 2025-03-10 10:12:21
   */
  bool SetContDrive1Times(uint8_t times);

  /**
   * @brief 设置第2次连续驱动的半循环次数
   * @param times 值范围：0~255
   * @return
   * @Date 2025-03-10 10:12:21
   */
  bool SetContDrive2Times(uint8_t times);

  /**
   * @brief 设置跟踪余量值，跟踪余量值越小，跟踪精度越高
   * @param value 值范围：0~255
   * @return
   * @Date 2025-03-10 10:19:55
   */
  bool SetContTrackMargin(uint8_t value);

  /**
   * @brief 设置制动器的半周期驱动时间（code/48K
   * s），该值必须小于一半F0的循环时间，
   * 建议将DRV_WIDTH配置为（24k/F0）-8-kTrackMargin-kBrkGain
   * @param value 值范围：0~255
   * @return
   * @Date 2025-03-10 10:27:30
   */
  bool SetContDriveWidth(uint8_t value);

  /**
   * @brief 获取f0检测的值
   * @return
   * @Date 2025-03-10 11:13:49
   */
  uint32_t GetF0Detection();

  /**
   * @brief 输入f0的值开始进行f0校准
   * @param f0_value GetF0Detection()函数获取的f0的值
   * @return
   * @Date 2025-03-10 11:17:27
   */
  bool SetF0Calibrate(uint32_t f0_value);

  /**
   * @brief 获取系统状态
   * @param status 使用System_Status结构体配置
   * @return
   * @Date 2025-03-10 14:01:45
   */
  bool GetSystemStatus(SystemStatus& status);

  /**
   * @brief 设置数字时钟启动
   * @param enable [true]：启动，[false]：关闭
   * @return
   * @Date 2025-03-10 14:30:42
   */
  bool SetClock(bool enable);

  /**
   * @brief 设置在RTP、kRam、TRIG模式下的增益
   * @param gain 值范围0~255
   * @return
   * @Date 2025-03-10 15:28:05
   */
  bool SetRrtModeGain(uint8_t gain);

  /**
   * @brief 初始化RAM模式
   * @param *waveform_data 波形数据指针
   * @param length 波形输出长度
   * @return
   * @Date 2025-03-10 16:20:23
   */
  bool InitRamMode(const uint8_t* waveform_data, size_t length);

  /**
   * @brief 设置RAM模式播放waveform数据
   * @param waveform_sequence_number
   * 值范围0~127，波形的序列号（该序列号为波形数据库里的序列号）
   * @param waveform_playback_count
   * 值范围0~15，波形的回放次数，当设置为15的时候为无限循环播放，当设置为无限循环播放的时候需要使用函数xxx进行停止播放
   * @param gain 值范围0~255，配置增益
   * @param auto_brake [true]：启动，[false]：关闭，自动制动
   * @param gain_bypass
   * [true]：在播放的时候改变增益，[false]：在播放的时候不改变增益，设置在播放的时候改变增益
   * @return
   * @Date 2025-03-10 15:35:46
   */
  bool RunRamPlaybackWaveform(uint8_t waveform_sequence_number,
      uint8_t waveform_playback_count, uint8_t gain = 127,
      bool auto_brake = true, bool gain_bypass = true);

  /**
   * @brief 设置停止标志位，设置该标志将停止当前的振动模式
   * @return
   * @Date 2025-03-10 17:40:58
   */
  bool SetStopFlag();

  /**
   * @brief 设置强制进入的模式
   * @param mode 使用Force_Mode::配置
   * @param enable [true]：启动，[false]：关闭
   * @return
   * @Date 2025-03-10 17:46:30
   */
  bool SetForceEnterMode(ForceMode mode, bool enable);

  /**
   * @brief RAM模式播放停止
   * @return
   * @Date 2025-03-10 17:55:04
   */
  bool StopRamPlaybackWaveform();

 private:
  enum class Cmd {
    kRoDeviceId = 0x64,

    kWoSrst = 0x00,
    kRoSysst,
    kRcSysint,
    kRwSysintm,
    kRoSyssst2,

    kRwPlaycfg2 = 0x07,
    kRwPlaycfg3,
    kRwPlaycfg4,
    kRwWavcfg1,
    kRwWavcfg2,
    kRwWavcfg3,
    kRwWavcfg4,
    kRwWavcfg5,
    kRwWavcfg6,
    kRwWavcfg7,
    kRwWavcfg8,
    kRwWavcfg9,
    kRwWavcfg10,
    kRwWavcfg11,
    kRwWavcfg12,
    kRwWavcfg13,

    kRwContcfg1 = 0x18,
    kRwContcfg2,
    kRwContcfg3,
    kRwContcfg4,
    kRwContcfg5,
    kRwContcfg6,
    kRwContcfg7,
    kRwContcfg8,
    kRwContcfg9,
    kRwContcfg10,
    kRwContcfg11,

    kRoContrd14 = 0x25,
    kRoContrd15,
    kRoContrd16,
    kRoContrd17,

    kRwRtpcfg1 = 0x2D,
    kRwRtpcfg2,
    kRwRtpcfg3,
    kRwRtpcfg4,
    kRwRtpcfg5,
    kRwRtpdata,
    kRwTrgcfg1,

    kRwTrgcfg4 = 0x36,
    kRwTrgcfg7 = 0x39,
    kRwTrgcfg8,

    kRwGlbcfg2 = 0x3C,
    kRwGlbcfg4 = 0x3E,
    kRoGlbrd5,
    kRwRamaddrh,
    kRwRamaddrl,
    kRwRamadata,
    kRwSysctrl1,
    kRwSysctrl2,

    kRwSysctrl7 = 0x49,
    kRwPwmcfg1 = 0x4C,
    kRwPwmcfg2,
    kRwPwmcfg3,
    kRwPwmcfg4,

    kRwDetcfg1 = 0x51,
    kRwDetcfg2,
    kRwDetRl,

    kRwDetVbat = 0x55,
    kRwDetLo = 0x57,
    kRwTrimcfg3 = 0x5A,
    kRwAnacfg8 = 0X77,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x58;

  int32_t rst_;
  uint32_t f0_value_ = 1700;
};
}  // namespace cpp_bus_driver
