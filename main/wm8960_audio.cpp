#if defined(ARDUINO_ARCH_RP2040)

#include "keyer_hardware.h"
#include "keyer_features_and_options_pico.h"
#include "keyer_pin_settings_pico.h"
#include "keyer_debug.h"

#if defined(FEATURE_WM8960_USB_AUDIO)

#include "wm8960_audio.h"
#include "wm8960_tinyusb_config.h"

#include <Arduino.h>
#include <CoreMutex.h>
#include "wm8960_i2s.h"
#include <USB.h>
#include <Wire.h>
#include <tusb.h>
#include <hardware/watchdog.h>
#include <hardware/structs/watchdog.h>
#include <hardware/exception.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>

#include <algorithm>
#include <cstring>

extern "C" void _exit(int status);

#if CFG_TUD_AUDIO < 2
#error "WM8960 USB audio requires two TinyUSB audio functions"
#endif
#if CFG_TUD_CDC < 2 || CFG_TUD_MIDI < 1
#error "WM8960 composite USB requires two CDC and one MIDI TinyUSB instances"
#endif
#if WM8960_MCLK_HZ != 24000000UL && WM8960_MCLK_HZ != 12288000UL
#error "WM8960 requires a configured 24 MHz or 12.288 MHz MCLK source"
#endif
#ifndef WM8960_USE_PLL
#define WM8960_USE_PLL (WM8960_MCLK_HZ == 24000000UL)
#endif
#if WM8960_USE_PLL && WM8960_MCLK_HZ != 24000000UL
#error "WM8960 PLL coefficients require a 24 MHz MCLK"
#endif

namespace {

__attribute__((naked)) void audio_diagnostic_hard_fault() {
  __asm volatile(
      "tst lr, #4\n"
      "ite eq\n"
      "mrseq r0, msp\n"
      "mrsne r0, psp\n"
      "ldr r1, =%c0\n"
      "ldr r2, =0x8962\n"
      "str r2, [r1]\n"
      "ldr r2, [r0, #20]\n"
      "str r2, [r1, #4]\n"
      "ldr r2, [r0, #24]\n"
      "str r2, [r1, #8]\n"
      "ldr r3, =%c1\n"
      "bic r3, r3, #1\n"
      "cmp r2, r3\n"
      "bne 2f\n"
      "ldr r2, [r0, #44]\n"
      "str r2, [r1, #4]\n"
      "str r4, [r1, #8]\n"
      "2:\n"
      "ldr r2, =0xe000ed28\n"
      "ldr r2, [r2]\n"
      "str r2, [r1, #12]\n"
      "1: b 1b\n"
      :
      : "i"(WATCHDOG_BASE + WATCHDOG_SCRATCH0_OFFSET),
        "i"(&_exit));
}

constexpr uint32_t kSampleRate = 48000;
constexpr uint16_t kMuteRampFrames = kSampleRate * 5 / 1000;
constexpr uint16_t kPlaybackRingFrames = 1024;
constexpr uint16_t kCaptureRingFrames = 1024;
constexpr int16_t kToneAmplitude = 8192;
constexpr uint8_t kSpeakerFunctionId = 0;
constexpr uint8_t kMicrophoneFunctionId = 1;
constexpr uint8_t kUacFeatureUnit = 0x02;
constexpr uint8_t kUacClockSource = 0x04;
constexpr uint8_t kUacClockEntity = 0x04;
constexpr size_t kSpeakerDescriptorLength =
    TUD_AUDIO_DESC_IAD_LEN + TUD_AUDIO_DESC_STD_AC_LEN +
    TUD_AUDIO_DESC_CS_AC_LEN + TUD_AUDIO_DESC_CLK_SRC_LEN +
    TUD_AUDIO_DESC_INPUT_TERM_LEN + TUD_AUDIO_DESC_OUTPUT_TERM_LEN +
    TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN +
    2 * TUD_AUDIO_DESC_STD_AS_INT_LEN + TUD_AUDIO_DESC_CS_AS_INT_LEN +
    TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN + TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN +
    TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN + TUD_AUDIO_DESC_STD_AS_ISO_FB_EP_LEN;
static_assert(kSpeakerDescriptorLength == CFG_TUD_AUDIO_FUNC_1_DESC_LEN,
              "speaker descriptor length must match TinyUSB configuration");
static_assert(TUD_AUDIO_MIC_ONE_CH_DESC_LEN == CFG_TUD_AUDIO_FUNC_2_DESC_LEN,
              "microphone descriptor length must match TinyUSB configuration");

struct AudioFunction {
  uint8_t string_index = 0;
  uint8_t control_interface = 0xFF;
  uint8_t data_endpoint = 0;
  uint8_t feedback_endpoint = 0;
};

struct CdcFunction {
  uint8_t string_index = 0;
  uint8_t notification_endpoint = 0;
  uint8_t out_endpoint = 0;
  uint8_t in_endpoint = 0;
};

struct MidiFunction {
  uint8_t string_index = 0;
  uint8_t out_endpoint = 0;
  uint8_t in_endpoint = 0;
};

AudioFunction speaker_function;
AudioFunction microphone_function;
CdcFunction second_cdc_function;
MidiFunction midi_function;
bool usb_registration_succeeded = false;

void speaker_descriptor_callback(int interface_number, uint8_t *destination,
                                 int length, void *parameter) {
  auto *function = static_cast<AudioFunction *>(parameter);
  function->control_interface = static_cast<uint8_t>(interface_number);

  const uint8_t descriptor[] = {
      TUD_AUDIO_DESC_IAD(static_cast<uint8_t>(interface_number), 0x02, 0x00),
      TUD_AUDIO_DESC_STD_AC(static_cast<uint8_t>(interface_number), 0x00,
                            function->string_index),
      TUD_AUDIO_DESC_CS_AC(
          0x0200, AUDIO_FUNC_DESKTOP_SPEAKER,
          TUD_AUDIO_DESC_CLK_SRC_LEN + TUD_AUDIO_DESC_INPUT_TERM_LEN +
              TUD_AUDIO_DESC_OUTPUT_TERM_LEN +
              TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN,
          AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS),
      TUD_AUDIO_DESC_CLK_SRC(
          kUacClockEntity, AUDIO_CLOCK_SOURCE_ATT_INT_PRO_CLK,
          AUDIO_CTRL_RW << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS, 0x01, 0x00),
      TUD_AUDIO_DESC_INPUT_TERM(
          0x01, AUDIO_TERM_TYPE_USB_STREAMING, 0x00, kUacClockEntity, 0x02,
          AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, 0x00, 0x00, 0x00),
      TUD_AUDIO_DESC_OUTPUT_TERM(
          0x03, AUDIO_TERM_TYPE_OUT_DESKTOP_SPEAKER, 0x01, kUacFeatureUnit,
          kUacClockEntity, 0x0000, 0x00),
      TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL(
          kUacFeatureUnit, 0x01,
          (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS) |
              (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS),
          (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS) |
              (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS),
          (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS) |
              (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS),
          0x00),
      TUD_AUDIO_DESC_STD_AS_INT(
          static_cast<uint8_t>(interface_number + 1), 0x00, 0x00, 0x00),
      TUD_AUDIO_DESC_STD_AS_INT(
          static_cast<uint8_t>(interface_number + 1), 0x01, 0x02, 0x00),
      TUD_AUDIO_DESC_CS_AS_INT(
          0x01, AUDIO_CTRL_NONE, AUDIO_FORMAT_TYPE_I, AUDIO_DATA_FORMAT_TYPE_I_PCM,
          0x02, AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, 0x00),
      TUD_AUDIO_DESC_TYPE_I_FORMAT(2, 16),
      TUD_AUDIO_DESC_STD_AS_ISO_EP(
          function->data_endpoint,
          static_cast<uint8_t>(TUSB_XFER_ISOCHRONOUS) |
              static_cast<uint8_t>(TUSB_ISO_EP_ATT_ASYNCHRONOUS) |
              static_cast<uint8_t>(TUSB_ISO_EP_ATT_DATA),
          CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX, 0x01),
      TUD_AUDIO_DESC_CS_AS_ISO_EP(
          AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, AUDIO_CTRL_NONE,
          AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, 0x0001),
      TUD_AUDIO_DESC_STD_AS_ISO_FB_EP(function->feedback_endpoint, 4, 0x01),
  };

  if (length == static_cast<int>(sizeof(descriptor))) {
    std::memcpy(destination, descriptor, sizeof(descriptor));
  }
}

void microphone_descriptor_callback(int interface_number,
                                    uint8_t *destination, int length,
                                    void *parameter) {
  auto *function = static_cast<AudioFunction *>(parameter);
  function->control_interface = static_cast<uint8_t>(interface_number);
  const uint8_t descriptor[] = {
      TUD_AUDIO_MIC_ONE_CH_DESCRIPTOR(
          static_cast<uint8_t>(interface_number), function->string_index, 2,
          16, function->data_endpoint, CFG_TUD_AUDIO_FUNC_2_EP_IN_SZ_MAX),
  };
  if (length == static_cast<int>(sizeof(descriptor))) {
    std::memcpy(destination, descriptor, sizeof(descriptor));
  }
}

void cdc_descriptor_callback(int interface_number, uint8_t *destination,
                             int length, void *parameter) {
  const auto *function = static_cast<CdcFunction *>(parameter);
  const uint8_t descriptor[] = {
      TUD_CDC_DESCRIPTOR(static_cast<uint8_t>(interface_number),
                         function->string_index,
                         function->notification_endpoint, 8,
                         function->out_endpoint, function->in_endpoint, 64),
  };
  if (length == static_cast<int>(sizeof(descriptor))) {
    std::memcpy(destination, descriptor, sizeof(descriptor));
  }
}

void midi_descriptor_callback(int interface_number, uint8_t *destination,
                              int length, void *parameter) {
  const auto *function = static_cast<MidiFunction *>(parameter);
  const uint8_t descriptor[] = {
      TUD_MIDI_DESCRIPTOR(static_cast<uint8_t>(interface_number),
                          function->string_index,
                          function->out_endpoint, function->in_endpoint, 64),
  };
  if (length == static_cast<int>(sizeof(descriptor))) {
    std::memcpy(destination, descriptor, sizeof(descriptor));
  }
}

class UsbCompositeRegistration {
 public:
  bool register_interfaces() {
    if (usb_registration_succeeded) {
      return true;
    }

    USB.disconnect();
    speaker_function.data_endpoint = USB.registerEndpointOut();
    speaker_function.feedback_endpoint = USB.registerEndpointIn();
    microphone_function.data_endpoint = USB.registerEndpointIn();
    second_cdc_function.notification_endpoint = USB.registerEndpointIn();
    second_cdc_function.in_endpoint = USB.registerEndpointIn();
    second_cdc_function.out_endpoint = USB.registerEndpointOut();
    midi_function.in_endpoint = USB.registerEndpointIn();
    midi_function.out_endpoint = USB.registerEndpointOut();

    if (speaker_function.data_endpoint == 0 ||
        speaker_function.feedback_endpoint == 0 ||
        microphone_function.data_endpoint == 0 ||
        second_cdc_function.notification_endpoint == 0 ||
        second_cdc_function.in_endpoint == 0 ||
        second_cdc_function.out_endpoint == 0 ||
        midi_function.in_endpoint == 0 || midi_function.out_endpoint == 0) {
      USB.connect();
      return false;
    }

    speaker_function.string_index = USB.registerString("WM8960 Speaker");
    microphone_function.string_index =
        USB.registerString("WM8960 Microphone");
    second_cdc_function.string_index =
        USB.registerString("Keyer CDC 2");
    midi_function.string_index = USB.registerString("Keyer MIDI");

    USB.registerInterface(2, speaker_descriptor_callback, &speaker_function,
                          CFG_TUD_AUDIO_FUNC_1_DESC_LEN, 10, 0);
    USB.registerInterface(2, microphone_descriptor_callback,
                          &microphone_function,
                          CFG_TUD_AUDIO_FUNC_2_DESC_LEN, 11, 0);
    USB.registerInterface(2, cdc_descriptor_callback, &second_cdc_function,
                          TUD_CDC_DESC_LEN, 20, 0);
    USB.registerInterface(2, midi_descriptor_callback, &midi_function,
                          TUD_MIDI_DESC_LEN, 30, 0);
    USB.connect();
    usb_registration_succeeded = true;
    return true;
  }
};

UsbCompositeRegistration usb_composite_registration;

constexpr uint8_t kCodecI2cController = (WM8960_I2C_SDA_PIN >> 1) & 1;
constexpr uint32_t kCodecI2cHz = 100000;
TwoWire &codec_bus = kCodecI2cController == 0 ? Wire : Wire1;

bool validate_codec_pins() {
  if (WM8960_I2C_SDA_PIN >= __GPIOCNT ||
      WM8960_I2C_SCL_PIN >= __GPIOCNT ||
      (WM8960_I2C_SDA_PIN & 1) != 0 ||
      (WM8960_I2C_SCL_PIN & 1) != 1 ||
      ((WM8960_I2C_SCL_PIN >> 1) & 1) != kCodecI2cController) {
    wm8960_audio_debug_port().println("[WM8960] invalid I2C SDA/SCL pin pair");
    return false;
  }
#if defined(I2CDEV_SDA_PIN) && defined(I2CDEV_SCL_PIN) && \
    (defined(FEATURE_MCP23017_EXPANDER) || defined(FEATURE_COMPASS))
  constexpr uint8_t keyer_i2c_controller =
#if defined(USE_WIRE1)
      1;
#else
      0;
#endif
  if (kCodecI2cController == keyer_i2c_controller &&
      (WM8960_I2C_SDA_PIN != I2CDEV_SDA_PIN ||
       WM8960_I2C_SCL_PIN != I2CDEV_SCL_PIN)) {
    wm8960_audio_debug_port().println("[WM8960] shared I2C controller requires the keyer SDA/SCL pins");
    return false;
  }
#endif
  const int lowest_pin = std::min<int>(
      WM8960_I2S_BCLK_PIN,
      std::min<int>(WM8960_I2S_DOUT_PIN, WM8960_I2S_DIN_PIN));
  const int highest_pin = std::max<int>(
      WM8960_I2S_LRCLK_PIN,
      std::max<int>(WM8960_I2S_DOUT_PIN, WM8960_I2S_DIN_PIN));
  if (WM8960_I2S_LRCLK_PIN != WM8960_I2S_BCLK_PIN + 1 ||
      lowest_pin < 0 || highest_pin >= __GPIOCNT ||
      (lowest_pin < 16 && highest_pin >= 32)) {
    wm8960_audio_debug_port().println("[WM8960] I2S needs adjacent BCLK/LRCLK and one PIO GPIO window");
    return false;
  }
  return true;
}

bool codec_ready = false;
bool initialization_attempted = false;
uint8_t failed_codec_register = 0xFF;
uint8_t codec_i2c_error = 0;
uint32_t codec_i2c_error_us = 0;
uint8_t codec_i2c_error_lines = 0;
volatile uint32_t i2s_receive_buffers = 0;
volatile uint32_t i2s_transmit_buffers = 0;

void trace_audio_stage(uint8_t stage) {
  watchdog_hw->scratch[0] =
      0x8963 | (static_cast<uint32_t>(stage) << 16) |
      ((i2s_receive_buffers & 0x3F) << 20) |
      ((i2s_transmit_buffers & 0x3F) << 26);
}

int16_t playback_ring[kPlaybackRingFrames * 2] = {};
uint32_t playback_read_index = 0;
uint32_t playback_write_index = 0;
int16_t capture_ring[kCaptureRingFrames] = {};
volatile uint32_t capture_read_index = 0;
volatile uint32_t capture_write_index = 0;

bool controls_initialized = false;
bool speaker_mute[3] = {};
bool microphone_mute[2] = {};
int16_t speaker_volume[3] = {};
int16_t microphone_volume[2] = {};
int32_t speaker_gain_q15[2] = {32767, 32767};
int32_t microphone_gain_q15 = 32767;
int32_t local_sidetone_gain_q15 = 32767;
int32_t local_playback_gain_q15 = 32767;
int32_t local_microphone_gain_q15 = 32767;
audio_control_range_2_n_t(1) speaker_volume_range[3] = {};
audio_control_range_2_n_t(1) microphone_volume_range[2] = {};
uint32_t speaker_sample_rate = kSampleRate;
uint32_t microphone_sample_rate = kSampleRate;

volatile bool tone_active = false;
volatile bool tone_mute_pending = false;
volatile uint32_t tone_stopped_ms = 0;
volatile uint32_t tone_mute_release_ms = 0;
uint16_t playback_mute_level = kMuteRampFrames;
uint16_t capture_mute_level = kMuteRampFrames;

uint16_t advance_mute_ramp(uint16_t level, bool muted) {
  if (muted && level > 0) {
    --level;
  } else if (!muted && level < kMuteRampFrames) {
    ++level;
  }
  return level;
}

int16_t apply_mute_ramp(int16_t sample, uint16_t level) {
  return static_cast<int16_t>(
      (static_cast<int32_t>(sample) * level) / kMuteRampFrames);
}

bool usb_audio_muted_for_keyer() {
  return tone_active ||
      (tone_mute_pending &&
       static_cast<uint32_t>(millis() - tone_stopped_ms) <
           tone_mute_release_ms);
}
uint32_t tone_phase = 0;
uint32_t tone_phase_increment = 0;
uint32_t tone_frames_remaining = 0;
uint32_t tone_start_count = 0;
uint32_t tone_output_frames = 0;
uint16_t last_tone_frequency = 0;
struct KeyTimingEdge {
  uint32_t time_us;
  uint16_t wpm;
  uint8_t element_dits;
  bool down;
};
constexpr uint16_t kKeyTimingEdges = 128;
KeyTimingEdge key_timing_edges[kKeyTimingEdges] = {};
uint32_t key_timing_count = 0;
uint32_t usb_playback_bytes = 0;
uint16_t usb_playback_peak = 0;
uint16_t adc_peak = 0;
volatile uint32_t capture_starved_packets = 0;
uint32_t playback_starved_frames = 0;
bool playback_started = false;
bool capture_primed = false;
constexpr uint32_t kPlaybackTargetFrames = 768;
int32_t playback_level_q8 = kPlaybackTargetFrames * 256;
uint32_t playback_feedback = 48UL << 16;
uint32_t playback_feedback_errors = 0;

uint16_t sample_magnitude(int16_t sample) {
  const int32_t value = sample;
  return static_cast<uint16_t>(value < 0 ? -value : value);
}

constexpr int16_t kSineTable[64] = {
    0,     3212,   6393,   9512,   12539,  15446,  18204,  20787,
    23170, 25329,  27245,  28898,  30273,  31356,  32137,  32609,
    32767, 32609,  32137,  31356,  30273,  28898,  27245,  25329,
    23170, 20787,  18204,  15446,  12539,  9512,   6393,   3212,
    0,     -3212,  -6393,  -9512,  -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539, -9512,  -6393,  -3212,
};

bool release_codec_clock() {
  gpio_set_dir(WM8960_I2C_SCL_PIN, GPIO_IN);
  const uint32_t start = micros();
  while (!gpio_get(WM8960_I2C_SCL_PIN)) {
    if (static_cast<uint32_t>(micros() - start) >= 500) {
      return false;
    }
  }
  delayMicroseconds(5);
  return true;
}

bool recover_codec_bus() {
  codec_bus.end();
  gpio_init(WM8960_I2C_SDA_PIN);
  gpio_init(WM8960_I2C_SCL_PIN);
  gpio_pull_up(WM8960_I2C_SDA_PIN);
  gpio_pull_up(WM8960_I2C_SCL_PIN);
  gpio_put(WM8960_I2C_SDA_PIN, 0);
  gpio_put(WM8960_I2C_SCL_PIN, 0);
  bool clock_released = release_codec_clock();
  // Release the bus between pulses; never drive a pulled-up line high.
  for (uint8_t pulse = 0;
       clock_released && pulse < 9 && !gpio_get(WM8960_I2C_SDA_PIN);
       ++pulse) {
    gpio_set_dir(WM8960_I2C_SCL_PIN, GPIO_OUT);
    delayMicroseconds(5);
    clock_released = release_codec_clock();
  }
  gpio_set_dir(WM8960_I2C_SCL_PIN, GPIO_OUT);
  gpio_set_dir(WM8960_I2C_SDA_PIN, GPIO_OUT);
  delayMicroseconds(5);
  clock_released = release_codec_clock() && clock_released;
  gpio_set_dir(WM8960_I2C_SDA_PIN, GPIO_IN);
  delayMicroseconds(5);
  const bool bus_released = clock_released &&
      gpio_get(WM8960_I2C_SDA_PIN) && gpio_get(WM8960_I2C_SCL_PIN);
  codec_bus.setClock(kCodecI2cHz);
  codec_bus.setTimeout(5);
  codec_bus.begin();
  if (!bus_released) {
    wm8960_audio_debug_port().printf("[WM8960] I2C recovery failed: SDA %u SCL %u\n",
                  static_cast<unsigned>(gpio_get(WM8960_I2C_SDA_PIN)),
                  static_cast<unsigned>(gpio_get(WM8960_I2C_SCL_PIN)));
  }
  return bus_released;
}

bool try_write_codec_register(uint8_t register_address, uint16_t value) {
  codec_bus.beginTransmission(WM8960_I2C_ADDRESS);
  const uint8_t first_byte =
      static_cast<uint8_t>((register_address << 1) | ((value >> 8) & 0x01));
  const uint8_t second_byte = static_cast<uint8_t>(value & 0xFF);
  if (codec_bus.write(first_byte) != 1 ||
      codec_bus.write(second_byte) != 1) {
    failed_codec_register = register_address;
    codec_i2c_error = 1;
    codec_bus.endTransmission();
    wm8960_audio_debug_port().printf("[WM8960] I2C write buffer error at register 0x%02X\n",
                  register_address);
    return false;
  }

  const uint32_t transfer_start = micros();
  const uint8_t result = codec_bus.endTransmission();
  if (result != 0) {
    failed_codec_register = register_address;
    codec_i2c_error = result;
    codec_i2c_error_us = micros() - transfer_start;
    codec_i2c_error_lines =
        (gpio_get(WM8960_I2C_SDA_PIN) ? 1 : 0) |
        (gpio_get(WM8960_I2C_SCL_PIN) ? 2 : 0);
    wm8960_audio_debug_port().printf("[WM8960] I2C error %u at register 0x%02X, %lu us, SDA %u SCL %u, functions %u/%u\n",
                  result, register_address,
                  static_cast<unsigned long>(codec_i2c_error_us),
                  static_cast<unsigned>(codec_i2c_error_lines & 1),
                  static_cast<unsigned>((codec_i2c_error_lines >> 1) & 1),
                  static_cast<unsigned>(gpio_get_function(WM8960_I2C_SDA_PIN)),
                  static_cast<unsigned>(gpio_get_function(WM8960_I2C_SCL_PIN)));
    return false;
  }
  return true;
}

bool write_codec_register(uint8_t register_address, uint16_t value) {
  if (try_write_codec_register(register_address, value)) {
    return true;
  }
#if defined(DEBUG_WM8960_AUDIO)
  wm8960_audio_debug_port().printf("[WM8960] recovering I2C bus before retrying register 0x%02X\n",
                register_address);
#endif
  if (!recover_codec_bus()) {
    return false;
  }
  if (!try_write_codec_register(register_address, value)) {
    return false;
  }
#if defined(DEBUG_WM8960_AUDIO)
  wm8960_audio_debug_port().printf("[WM8960] I2C recovery succeeded for register 0x%02X\n",
                register_address);
#endif
  return true;
}

bool initialize_codec_registers() {
  if (!write_codec_register(0x0F, 0x000) ||
      !write_codec_register(0x05, 0x008) ||
      !write_codec_register(0x1C, 0x09C) ||
      !write_codec_register(0x19, 0x080)) {
    return false;
  }
  delay(100);
#if WM8960_USE_PLL
  // 24 MHz / 2 * (8 + 0x3126E9 / 2^24) / 4 = 24.576 MHz PLL output.
  if (!write_codec_register(0x04, 0x000) ||
      !write_codec_register(0x1A, 0x000) ||
      !write_codec_register(0x35, 0x031) ||
      !write_codec_register(0x36, 0x026) ||
      !write_codec_register(0x37, 0x0E9) ||
      !write_codec_register(0x34, 0x038) ||
      !write_codec_register(0x1A, 0x001)) {
    return false;
  }
  delay(250);
  constexpr uint16_t clock_configuration = 0x005;  // PLL / 2 = 256 * 48 kHz.
  constexpr uint16_t output_power = 0x1F9;  // Keep the PLL powered with the outputs.
#else
  constexpr uint16_t clock_configuration = 0x000;
  constexpr uint16_t output_power = 0x1F8;
#endif
  if (!write_codec_register(0x19, 0x0C0) ||
      !write_codec_register(0x1C, 0x008) ||
      !write_codec_register(0x19, 0x140) ||
      !write_codec_register(0x04, clock_configuration) ||
      !write_codec_register(0x07, 0x002) ||
      !write_codec_register(0x0A, 0x0FF) ||
      !write_codec_register(0x0B, 0x1FF) ||
      !write_codec_register(0x22, 0x100) ||
      !write_codec_register(0x25, 0x100) ||
      !write_codec_register(0x2F, 0x03C) ||
      !write_codec_register(0x31, 0x0C0) ||
      !write_codec_register(0x1A, output_power) ||
      !write_codec_register(0x02, 0x078) ||
      !write_codec_register(0x03, 0x178) ||
      !write_codec_register(0x28, 0x078) ||
      !write_codec_register(0x29, 0x178)) {
    return false;
  }
  delay(5);

  if (!write_codec_register(0x20, 0x138) ||
      !write_codec_register(0x21, 0x138) ||
      !write_codec_register(0x00, 0x020) ||
      !write_codec_register(0x01, 0x120) ||
      !write_codec_register(0x15, 0x0C3) ||
      !write_codec_register(0x16, 0x1C3) ||
      !write_codec_register(0x2F, 0x03C) ||
      !write_codec_register(0x19, 0x17E)) {
    return false;
  }
  delay(100);
  return true;
}

int32_t volume_to_gain_q15(int32_t volume_db_8_8) {
  if (volume_db_8_8 >= 0) {
    return 32767;
  }
  int32_t attenuation_steps = (-volume_db_8_8 + 128) / 256;
  attenuation_steps = std::min<int32_t>(attenuation_steps, 50);
  int32_t gain = 32767;
  while (attenuation_steps-- > 0) {
    gain = (gain * 29205 + 16384) >> 15;
  }
  return gain;
}

void update_audio_gains() {
  for (uint8_t channel = 0; channel < 2; ++channel) {
    const uint8_t control_channel = channel + 1;
    const bool muted =
        speaker_mute[0] || speaker_mute[control_channel];
    const int32_t volume_db =
        static_cast<int32_t>(speaker_volume[0]) +
        static_cast<int32_t>(speaker_volume[control_channel]);
    speaker_gain_q15[channel] =
        muted ? 0 : volume_to_gain_q15(volume_db);
  }

  microphone_gain_q15 =
      microphone_mute[0] || microphone_mute[1]
          ? 0
          : volume_to_gain_q15(
                static_cast<int32_t>(microphone_volume[0]) +
                static_cast<int32_t>(microphone_volume[1]));
}

void initialize_audio_controls() {
  if (controls_initialized) {
    return;
  }
  controls_initialized = true;

  for (uint8_t channel = 0; channel < 3; ++channel) {
    speaker_volume_range[channel].wNumSubRanges = 1;
    speaker_volume_range[channel].subrange[0].bMin = -50 * 256;
    speaker_volume_range[channel].subrange[0].bMax = 0;
    speaker_volume_range[channel].subrange[0].bRes = 256;
  }
  for (uint8_t channel = 0; channel < 2; ++channel) {
    microphone_volume_range[channel].wNumSubRanges = 1;
    microphone_volume_range[channel].subrange[0].bMin = -50 * 256;
    microphone_volume_range[channel].subrange[0].bMax = 0;
    microphone_volume_range[channel].subrange[0].bRes = 256;
  }
  update_audio_gains();
}

bool is_microphone_interface(uint8_t interface_number) {
  return interface_number == microphone_function.control_interface;
}

bool is_speaker_interface(uint8_t interface_number) {
  return interface_number == speaker_function.control_interface;
}

void capture_i2s_samples() {
  static int16_t samples[512];
  int bytes_available = wm8960_i2s::available();
  bytes_available = std::min<int>(bytes_available, sizeof(samples));
  bytes_available &= ~0x03;
  if (bytes_available <= 0) {
    return;
  }

  const size_t bytes_read =
      wm8960_i2s::read(reinterpret_cast<uint8_t *>(samples), bytes_available);
  const size_t frames_read = (bytes_read & ~static_cast<size_t>(0x03)) / 4;
  for (size_t frame = 0; frame < frames_read; ++frame) {
    adc_peak = std::max(adc_peak, sample_magnitude(samples[frame * 2]));
    adc_peak = std::max(adc_peak, sample_magnitude(samples[frame * 2 + 1]));
    const uint32_t write_index = capture_write_index;
    if (write_index - capture_read_index >= kCaptureRingFrames) {
      continue;
    }
    capture_ring[write_index & (kCaptureRingFrames - 1)] = samples[frame * 2];
    capture_write_index = write_index + 1;
  }
}

// Caller owns USB.mutex (USB callbacks already run under it).
void capture_usb_playback_locked() {
  if (!tud_mounted()) {
    return;
  }

  static int16_t packet[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX / sizeof(int16_t)];
  for (uint8_t reads = 0; reads < 4; ++reads) {
    const uint16_t available = tud_audio_n_available(kSpeakerFunctionId);
    const uint32_t queued_frames =
        playback_write_index - playback_read_index;
    const uint32_t free_frames = kPlaybackRingFrames - queued_frames;
    if (available < 4 || free_frames == 0) {
      break;
    }

    uint16_t bytes_to_read =
        std::min<uint16_t>(available, std::min<uint32_t>(sizeof(packet),
                                                       free_frames * 4));
    bytes_to_read &= static_cast<uint16_t>(~0x03);
    if (bytes_to_read == 0) {
      break;
    }
    const uint16_t bytes_read =
        tud_audio_n_read(kSpeakerFunctionId, packet, bytes_to_read);
    usb_playback_bytes += bytes_read;
    for (uint16_t sample = 0; sample < bytes_read / sizeof(int16_t); ++sample) {
      usb_playback_peak =
          std::max(usb_playback_peak, sample_magnitude(packet[sample]));
    }
    const uint16_t frames_read = bytes_read / 4;
    for (uint16_t frame = 0; frame < frames_read; ++frame) {
      playback_ring[((playback_write_index + frame) &
                     (kPlaybackRingFrames - 1)) *
                        2] =
          static_cast<int16_t>(
              (static_cast<int32_t>(packet[frame * 2]) *
               speaker_gain_q15[0]) /
              32767);
      playback_ring[((playback_write_index + frame) &
                     (kPlaybackRingFrames - 1)) *
                        2 +
                    1] =
          static_cast<int16_t>(
              (static_cast<int32_t>(packet[frame * 2 + 1]) *
               speaker_gain_q15[1]) /
              32767);
    }
    playback_write_index += frames_read;
    if (frames_read == 0) {
      break;
    }
  }
}

void capture_usb_playback() {
  CoreMutex usb_lock(&USB.mutex, false);
  if (usb_lock) {
    capture_usb_playback_locked();
  }
}

int16_t next_tone_sample(uint32_t phase) {
  const uint8_t index = static_cast<uint8_t>(phase >> 26);
  const uint8_t next_index = static_cast<uint8_t>((index + 1) & 0x3F);
  const uint32_t fraction = (phase >> 10) & 0xFFFF;
  const int32_t first = kSineTable[index];
  const int32_t difference =
      static_cast<int32_t>(kSineTable[next_index]) - first;
  const int32_t interpolated =
      first + ((difference * static_cast<int32_t>(fraction)) >> 16);
  return static_cast<int16_t>((interpolated * kToneAmplitude) / 32767);
}

void render_i2s_output() {
  if (!codec_ready) {
    return;
  }

  const int writable_bytes = wm8960_i2s::available_for_write();
  const uint16_t frame_count = static_cast<uint16_t>(
      std::min<int>(writable_bytes / 4, 128));
  if (frame_count == 0) {
    return;
  }

  static int16_t output[256];
  uint32_t phase = tone_phase;
  uint32_t tone_remaining = tone_frames_remaining;
  bool active_tone = tone_active;
  const bool mute_for_keyer = usb_audio_muted_for_keyer();
  uint16_t mute_level = playback_mute_level;
  uint16_t tone_frames_generated = 0;
  const uint32_t playback_queued = playback_write_index - playback_read_index;
  if (!playback_started && playback_queued >= 192) {
    playback_started = true;
  }
  const uint32_t playable_frames = playback_started ? playback_queued : 0;

  for (uint16_t frame = 0; frame < frame_count; ++frame) {
    mute_level = advance_mute_ramp(mute_level, mute_for_keyer);
    int16_t left = 0;
    int16_t right = 0;
    if (frame < playable_frames) {
      const uint16_t ring_frame =
          (playback_read_index + frame) & (kPlaybackRingFrames - 1);
      left = apply_mute_ramp(static_cast<int16_t>(
          (static_cast<int32_t>(playback_ring[ring_frame * 2]) *
           local_playback_gain_q15) / 32767), mute_level);
      right = apply_mute_ramp(static_cast<int16_t>(
          (static_cast<int32_t>(playback_ring[ring_frame * 2 + 1]) *
           local_playback_gain_q15) / 32767), mute_level);
    }

    if (active_tone) {
      const int16_t tone = static_cast<int16_t>(
          (static_cast<int32_t>(next_tone_sample(phase)) *
           local_sidetone_gain_q15) / 32767);
      left = static_cast<int16_t>(std::max<int32_t>(
          -32768, std::min<int32_t>(32767, static_cast<int32_t>(left) + tone)));
      right = static_cast<int16_t>(std::max<int32_t>(
          -32768, std::min<int32_t>(32767, static_cast<int32_t>(right) + tone)));
      phase += tone_phase_increment;
      ++tone_frames_generated;
      if (tone_remaining > 0 && --tone_remaining == 0) {
        active_tone = false;
      }
    }
    output[frame * 2] = left;
    output[frame * 2 + 1] = right;
  }

  const size_t bytes_written = wm8960_i2s::write(
      reinterpret_cast<const uint8_t *>(output), frame_count * 4);
  const uint16_t frames_written =
      static_cast<uint16_t>(bytes_written / 4);
  for (uint16_t frame = 0; frame < frames_written; ++frame) {
    playback_mute_level =
        advance_mute_ramp(playback_mute_level, mute_for_keyer);
  }
  playback_read_index +=
      std::min<uint32_t>(frames_written, playable_frames);
  if (playback_started && frames_written > playable_frames) {
    playback_starved_frames += frames_written - playable_frames;
    playback_started = false;
  }

  const uint16_t accepted_tone_frames =
      std::min<uint16_t>(frames_written, tone_frames_generated);
  tone_output_frames += accepted_tone_frames;
  tone_phase += tone_phase_increment * accepted_tone_frames;
  if (tone_frames_remaining > 0) {
    if (accepted_tone_frames >= tone_frames_remaining) {
      tone_frames_remaining = 0;
      wm8960_audio_stop_tone();
    } else {
      tone_frames_remaining -= accepted_tone_frames;
    }
  }
}

}  // namespace

Wm8960TerminalSerial Wm8960Terminal;

HardwareSerial &wm8960_audio_debug_port() {
#if defined(FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT)
  return Wm8960Terminal;
#else
  return Serial;
#endif
}

void Wm8960TerminalSerial::begin(unsigned long baud) {
  (void)baud;
  running_ = true;
}

void Wm8960TerminalSerial::begin(unsigned long baud, uint16_t config) {
  (void)config;
  begin(baud);
}

void Wm8960TerminalSerial::end() {
  running_ = false;
}

int Wm8960TerminalSerial::available() {
  CoreMutex lock(&USB.mutex, false);
  return running_ && usb_registration_succeeded && lock
      ? tud_cdc_n_available(1) : 0;
}

int Wm8960TerminalSerial::availableForWrite() {
  CoreMutex lock(&USB.mutex, false);
  return running_ && usb_registration_succeeded && lock
      ? tud_cdc_n_write_available(1) : 0;
}

int Wm8960TerminalSerial::peek() {
  CoreMutex lock(&USB.mutex, false);
  uint8_t value;
  return running_ && usb_registration_succeeded && lock &&
      tud_cdc_n_peek(1, &value) ? value : -1;
}

int Wm8960TerminalSerial::read() {
  CoreMutex lock(&USB.mutex, false);
  return running_ && usb_registration_succeeded && lock
      ? tud_cdc_n_read_char(1) : -1;
}

void Wm8960TerminalSerial::flush() {
  CoreMutex lock(&USB.mutex, false);
  if (running_ && usb_registration_succeeded && lock) {
    tud_cdc_n_write_flush(1);
  }
}

size_t Wm8960TerminalSerial::write(uint8_t value) {
  return write(&value, 1);
}

size_t Wm8960TerminalSerial::write(const uint8_t *buffer, size_t size) {
  size_t written = 0;
  uint32_t last_progress = millis();
  while (written < size) {
    {
      CoreMutex lock(&USB.mutex, false);
      if (!running_ || !usb_registration_succeeded || !lock ||
          !tud_cdc_n_connected(1)) {
        break;
      }
      const size_t count = tud_cdc_n_write(1, buffer + written, size - written);
      tud_cdc_n_write_flush(1);
      written += count;
      if (count > 0) {
        last_progress = millis();
      }
    }
    if (written == size || millis() - last_progress >= 1000) {
      break;
    }
    yield();
  }
  if (written != size) {
    setWriteError();
  }
  return written;
}

Wm8960TerminalSerial::operator bool() {
  CoreMutex lock(&USB.mutex, false);
  return running_ && usb_registration_succeeded && lock &&
      tud_cdc_n_connected(1);
}

bool wm8960_audio_set_levels(int sidetone, int playback, int microphone,
                            bool sidetone_muted, bool playback_muted,
                            bool microphone_muted) {
  if (sidetone < 0 || sidetone > 100 || playback < 0 || playback > 100 ||
      microphone < 0 || microphone > 100) {
    wm8960_audio_debug_port().println("[WM8960] invalid saved audio level (expected 0..100)");
    return false;
  }
  const uint32_t interrupt_state = save_and_disable_interrupts();
  local_sidetone_gain_q15 = sidetone_muted ? 0 : sidetone * 32767 / 100;
  local_playback_gain_q15 = playback_muted ? 0 : playback * 32767 / 100;
  local_microphone_gain_q15 = microphone_muted ? 0 : microphone * 32767 / 100;
  restore_interrupts(interrupt_state);
  return true;
}

void wm8960_audio_trace_keyer(uint8_t stage) {
  trace_audio_stage(stage);
}

void wm8960_audio_trace_key_edge(bool down, uint8_t element_dits,
                                 uint16_t wpm) {
  key_timing_edges[key_timing_count % kKeyTimingEdges] =
      {micros(), wpm, element_dits, down};
  ++key_timing_count;
}

extern "C" void tud_event_hook_cb(uint8_t rhport, uint32_t event_id,
                                 bool in_isr) {
  (void)rhport;
  (void)event_id;
  (void)in_isr;
  // The core's 1 ms polling fallback can miss the next ISO packet deadline.
  // Wake its existing mutex-protected task as soon as an event is queued.
  if (initialization_attempted) {
    irq_set_pending(USB.usbTaskIRQ);
  }
}

bool wm8960_audio_initialize(void (*report_progress)(const char *)) {
  const uint32_t retained_trace = watchdog_hw->scratch[0];
  const uint16_t retained_marker = retained_trace & 0xFFFF;
  if (watchdog_caused_reboot() &&
      (retained_marker == 0x8961 || retained_marker == 0x8962 ||
       retained_marker == 0x8963)) {
    char status[48];
    if (report_progress) {
      if (retained_marker == 0x8963) {
        snprintf(status, sizeof(status), "WD S:%lu R:%lu T:%lu",
                 static_cast<unsigned long>((retained_trace >> 16) & 0x0F),
                 static_cast<unsigned long>((retained_trace >> 20) & 0x3F),
                 static_cast<unsigned long>((retained_trace >> 26) & 0x3F));
        report_progress(status);
        snprintf(status, sizeof(status), "USB:%lx IV:%08lx",
                 static_cast<unsigned long>(watchdog_hw->scratch[1]),
                 static_cast<unsigned long>(watchdog_hw->scratch[2]));
        report_progress(status);
      } else {
        const uint32_t address = watchdog_hw->scratch[2];
        if (address >= XIP_BASE && address < XIP_BASE + 0x00400000 &&
            watchdog_hw->scratch[3] == 0) {
          snprintf(status, sizeof(status), "Panic: %.32s",
                   reinterpret_cast<const char *>(address));
        } else {
          snprintf(status, sizeof(status), "PC/IV: %08lx",
                   static_cast<unsigned long>(address));
        }
        report_progress(status);
        snprintf(status, sizeof(status), "LR: %08lx",
                 static_cast<unsigned long>(watchdog_hw->scratch[1]));
        report_progress(status);
      }
    }
    watchdog_hw->scratch[0] = 0;
    while (true) {
      delay(100);
    }
  }
  trace_audio_stage(1);
  watchdog_hw->scratch[1] = 0;
  watchdog_hw->scratch[2] = 0;
  watchdog_hw->scratch[3] = 0;
  exception_set_exclusive_handler(HARDFAULT_EXCEPTION,
                                  audio_diagnostic_hard_fault);
  watchdog_enable(8000, true);
  initialization_attempted = true;
#if defined(DEBUG_WM8960_AUDIO)
  if (report_progress) {
    report_progress("WM8960: USB setup");
  }
#endif
  if (!usb_composite_registration.register_interfaces()) {
    wm8960_audio_debug_port().println("[WM8960] USB composite endpoint registration failed");
    return false;
  }

#if defined(DEBUG_WM8960_AUDIO)
  if (report_progress) {
    report_progress("WM8960: I2C setup");
  }
#endif
  if (!validate_codec_pins()) {
    return false;
  }
  if (!codec_bus.setSDA(WM8960_I2C_SDA_PIN) ||
      !codec_bus.setSCL(WM8960_I2C_SCL_PIN)) {
    wm8960_audio_debug_port().println("[WM8960] I2C pin configuration failed");
    return false;
  }
  codec_bus.setClock(kCodecI2cHz);
  codec_bus.setTimeout(5);
  codec_bus.begin();

#if defined(DEBUG_WM8960_AUDIO)
  if (report_progress) {
    report_progress("WM8960: codec setup");
  }
#endif
  if (!initialize_codec_registers()) {
    wm8960_audio_debug_port().println("[WM8960] codec initialization failed");
    return false;
  }
  // Finish control-bus setup before starting continuous I2S clock activity.
  if (!write_codec_register(0x05, 0x000)) {
    wm8960_audio_debug_port().println("[WM8960] failed to unmute DAC");
    return false;
  }

#if defined(DEBUG_WM8960_AUDIO)
  if (report_progress) {
    report_progress("WM8960: I2S setup");
  }
#endif
  const bool i2s_started = wm8960_i2s::begin(kSampleRate);
  if (!i2s_started) {
    wm8960_audio_debug_port().println("[WM8960] I2S initialization failed");
    return false;
  }

#if defined(DEBUG_WM8960_AUDIO)
  if (report_progress) {
    report_progress("WM8960: I2S started");
  }
#endif
  codec_ready = true;
#if defined(DEBUG_WM8960_AUDIO)
  if (report_progress) {
    report_progress("WM8960: ready");
  }
  wm8960_audio_debug_port().printf("[WM8960] duplex I2S ready at %lu Hz, MCLK %lu Hz\n",
                static_cast<unsigned long>(kSampleRate),
                static_cast<unsigned long>(WM8960_MCLK_HZ));
  wm8960_audio_debug_port().printf("[WM8960] codec clock: %s\n",
                WM8960_USE_PLL ? "PLL" : "direct MCLK (PLL bypass)");
#endif
  trace_audio_stage(2);
  return true;
}

void wm8960_audio_service(void) {
  if (get_core_num() != 0 || !initialization_attempted) {
    return;
  }
  watchdog_update();
  trace_audio_stage(3);
  static bool serial_was_connected = false;
  const bool serial_connected = static_cast<bool>(wm8960_audio_debug_port());
  if (serial_connected && !serial_was_connected) {
    serial_was_connected = true;
#if defined(DEBUG_WM8960_AUDIO)
    const uint32_t edge_count =
        std::min<uint32_t>(key_timing_count, kKeyTimingEdges);
    wm8960_audio_debug_port().println("[TIMING] time_us,set_wpm,element_dits,down");
    for (uint32_t edge = key_timing_count - edge_count;
         edge < key_timing_count; ++edge) {
      const KeyTimingEdge &sample = key_timing_edges[edge % kKeyTimingEdges];
      wm8960_audio_debug_port().printf("[TIMING] %lu,%u,%u,%u\n",
                    static_cast<unsigned long>(sample.time_us), sample.wpm,
                    sample.element_dits, static_cast<unsigned>(sample.down));
    }
    uint32_t clock_changes = 0;
    bool clock_level = gpio_get(WM8960_I2S_LRCLK_PIN);
    const uint32_t clock_test_start = micros();
    while (micros() - clock_test_start < 2000) {
      const bool level = gpio_get(WM8960_I2S_LRCLK_PIN);
      if (level != clock_level) {
        ++clock_changes;
        clock_level = level;
      }
    }
    wm8960_audio_debug_port().printf("[WM8960] LRCLK transitions/2ms %lu, USB playback bytes %lu peak %u, ADC peak %u, I2C GPIO functions %u/%u\n",
                  static_cast<unsigned long>(clock_changes),
                  static_cast<unsigned long>(usb_playback_bytes),
                  usb_playback_peak, adc_peak,
                  static_cast<unsigned>(gpio_get_function(WM8960_I2C_SDA_PIN)),
                  static_cast<unsigned>(gpio_get_function(WM8960_I2C_SCL_PIN)));
    wm8960_audio_debug_port().printf("[WM8960] audio %s, sample rate %lu Hz, MCLK %lu Hz, DMA RX %lu TX %lu\n",
                  codec_ready ? "ready" : "initialization failed",
                  static_cast<unsigned long>(kSampleRate),
                  static_cast<unsigned long>(WM8960_MCLK_HZ),
                  static_cast<unsigned long>(i2s_receive_buffers),
                  static_cast<unsigned long>(i2s_transmit_buffers));
    wm8960_audio_debug_port().printf("[WM8960] tones %lu, frequency %u Hz, tone frames %lu, GPIO functions BCLK/LRCLK/DOUT/DIN %u/%u/%u/%u\n",
                  static_cast<unsigned long>(tone_start_count),
                  last_tone_frequency,
                  static_cast<unsigned long>(tone_output_frames),
                  static_cast<unsigned>(gpio_get_function(WM8960_I2S_BCLK_PIN)),
                  static_cast<unsigned>(gpio_get_function(WM8960_I2S_LRCLK_PIN)),
                  static_cast<unsigned>(gpio_get_function(WM8960_I2S_DOUT_PIN)),
                  static_cast<unsigned>(gpio_get_function(WM8960_I2S_DIN_PIN)));
    wm8960_audio_debug_port().printf("[WM8960] codec clock: %s\n",
                  WM8960_USE_PLL ? "PLL" : "direct MCLK (PLL bypass)");
    wm8960_audio_debug_port().printf("[WM8960] playback missing frames %lu, capture short packets %lu, PIO underruns %lu, max service gap %lu us\n",
                  static_cast<unsigned long>(playback_starved_frames),
                  static_cast<unsigned long>(capture_starved_packets),
                  static_cast<unsigned long>(wm8960_i2s::underruns()),
                  static_cast<unsigned long>(wm8960_i2s::max_service_gap_us()));
    wm8960_audio_debug_port().printf("[WM8960] playback queue average %ld frames, feedback %lu/65536 samples/ms, feedback errors %lu\n",
                  static_cast<long>(playback_level_q8 / 256),
                  static_cast<unsigned long>(playback_feedback),
                  static_cast<unsigned long>(playback_feedback_errors));
#endif
    if (!codec_ready) {
      wm8960_audio_debug_port().println("[WM8960] audio initialization failed");
    }
    if (failed_codec_register != 0xFF) {
      wm8960_audio_debug_port().printf("[WM8960] last codec write error: register 0x%02X, I2C error %u, %lu us, SDA %u SCL %u, bus %lu Hz\n",
                    failed_codec_register, codec_i2c_error,
                    static_cast<unsigned long>(codec_i2c_error_us),
                    static_cast<unsigned>(codec_i2c_error_lines & 1),
                    static_cast<unsigned>((codec_i2c_error_lines >> 1) & 1),
                    static_cast<unsigned long>(kCodecI2cHz));
    }
  }
  serial_was_connected = serial_connected;
  trace_audio_stage(4);
  initialize_audio_controls();
  trace_audio_stage(5);
  capture_usb_playback();
  trace_audio_stage(6);
  if (codec_ready) {
    trace_audio_stage(7);
    CoreMutex usb_lock(&USB.mutex, false);
    if (usb_lock) {
      render_i2s_output();
    }
  }
  i2s_transmit_buffers = wm8960_i2s::transmitted_frames() / 128;
  trace_audio_stage(8);
}

void wm8960_audio_start_tone(uint16_t frequency, uint32_t duration_ms,
                             uint32_t mute_release_ms) {
  if (!codec_ready || frequency == 0) {
    return;
  }

  const uint32_t interrupt_state = save_and_disable_interrupts();
  ++tone_start_count;
  tone_mute_release_ms = mute_release_ms;
  tone_mute_pending = true;
  last_tone_frequency = frequency;
  tone_phase = 0;
  tone_phase_increment = static_cast<uint32_t>(
      (static_cast<uint64_t>(frequency) << 32) / kSampleRate);
  const uint64_t frame_duration =
      static_cast<uint64_t>(duration_ms) * kSampleRate / 1000;
  tone_frames_remaining =
      duration_ms == 0
          ? 0
          : static_cast<uint32_t>(
                std::min<uint64_t>(frame_duration, UINT32_MAX));
  tone_active = true;
  restore_interrupts(interrupt_state);
}

void wm8960_audio_stop_tone(void) {
  const uint32_t interrupt_state = save_and_disable_interrupts();
  if (tone_active) {
    tone_stopped_ms = millis();
  }
  tone_active = false;
  tone_frames_remaining = 0;
  restore_interrupts(interrupt_state);
}

bool tud_audio_set_itf_cb(uint8_t rhport,
                          tusb_control_request_t const *request) {
  (void)rhport;
  (void)request;
  if (static_cast<uint8_t>(request->wIndex) ==
      speaker_function.control_interface + 1) {
    playback_read_index = playback_write_index = 0;
    playback_started = false;
    playback_level_q8 = kPlaybackTargetFrames * 256;
    playback_feedback = 48UL << 16;
  }
  if (static_cast<uint8_t>(request->wIndex) ==
      microphone_function.control_interface + 1) {
    capture_read_index = capture_write_index;
    capture_primed = false;
  }
  initialize_audio_controls();
  return true;
}

bool tud_audio_set_itf_close_EP_cb(
    uint8_t rhport, tusb_control_request_t const *request) {
  (void)rhport;
  (void)request;
  return true;
}

bool tud_audio_set_req_ep_cb(uint8_t rhport,
                             tusb_control_request_t const *request,
                             uint8_t *buffer) {
  (void)rhport;
  (void)request;
  (void)buffer;
  return false;
}

bool tud_audio_set_req_itf_cb(uint8_t rhport,
                              tusb_control_request_t const *request,
                              uint8_t *buffer) {
  (void)rhport;
  (void)request;
  (void)buffer;
  return false;
}

bool tud_audio_set_req_entity_cb(uint8_t rhport,
                                 tusb_control_request_t const *request,
                                 uint8_t *buffer) {
  initialize_audio_controls();
  const uint8_t channel = TU_U16_LOW(request->wValue);
  const uint8_t selector = TU_U16_HIGH(request->wValue);
  const uint8_t entity = TU_U16_HIGH(request->wIndex);
  const uint8_t interface_number = TU_U16_LOW(request->wIndex);

  if (entity == kUacFeatureUnit && request->bRequest == AUDIO_CS_REQ_CUR) {
    if (is_speaker_interface(interface_number) && channel < 3) {
      if (selector == AUDIO_FU_CTRL_MUTE &&
          request->wLength == sizeof(audio_control_cur_1_t)) {
        speaker_mute[channel] =
            static_cast<audio_control_cur_1_t *>(static_cast<void *>(buffer))
                ->bCur != 0;
        update_audio_gains();
        return true;
      }
      if (selector == AUDIO_FU_CTRL_VOLUME &&
          request->wLength == sizeof(audio_control_cur_2_t)) {
        speaker_volume[channel] =
            static_cast<audio_control_cur_2_t *>(static_cast<void *>(buffer))
                ->bCur;
        update_audio_gains();
        return true;
      }
    }
    if (is_microphone_interface(interface_number) && channel < 2) {
      if (selector == AUDIO_FU_CTRL_MUTE &&
          request->wLength == sizeof(audio_control_cur_1_t)) {
        microphone_mute[channel] =
            static_cast<audio_control_cur_1_t *>(static_cast<void *>(buffer))
                ->bCur != 0;
        update_audio_gains();
        return true;
      }
      if (selector == AUDIO_FU_CTRL_VOLUME &&
          request->wLength == sizeof(audio_control_cur_2_t)) {
        microphone_volume[channel] =
            static_cast<audio_control_cur_2_t *>(static_cast<void *>(buffer))
                ->bCur;
        update_audio_gains();
        return true;
      }
    }
  }

  if (entity == kUacClockSource && selector == AUDIO_CS_CTRL_SAM_FREQ &&
      request->bRequest == AUDIO_CS_REQ_CUR &&
      request->wLength == sizeof(audio_control_cur_4_t)) {
    const uint32_t requested_rate =
        static_cast<audio_control_cur_4_t *>(static_cast<void *>(buffer))
            ->bCur;
    if (requested_rate != kSampleRate) {
      return false;
    }
    if (is_speaker_interface(interface_number)) {
      speaker_sample_rate = requested_rate;
      return true;
    }
    if (is_microphone_interface(interface_number)) {
      microphone_sample_rate = requested_rate;
      return true;
    }
  }

  (void)rhport;
  return false;
}

bool tud_audio_get_req_entity_cb(
    uint8_t rhport, tusb_control_request_t const *request) {
  initialize_audio_controls();
  const uint8_t channel = TU_U16_LOW(request->wValue);
  const uint8_t selector = TU_U16_HIGH(request->wValue);
  const uint8_t entity = TU_U16_HIGH(request->wIndex);
  const uint8_t interface_number = TU_U16_LOW(request->wIndex);

  const bool speaker = is_speaker_interface(interface_number);
  const bool microphone = is_microphone_interface(interface_number);
  if (microphone && entity == 0x01 &&
      selector == AUDIO_TE_CTRL_CONNECTOR &&
      request->bRequest == AUDIO_CS_REQ_CUR) {
    audio_desc_channel_cluster_t connector = {};
    connector.bNrChannels = 1;
    connector.bmChannelConfig = AUDIO_CHANNEL_CONFIG_NON_PREDEFINED;
    return tud_audio_buffer_and_schedule_control_xfer(
        rhport, request, &connector, sizeof(connector));
  }
  if (entity == kUacFeatureUnit && (speaker || microphone)) {
    const uint8_t max_channel = speaker ? 3 : 2;
    if (channel >= max_channel) {
      return false;
    }
    if (selector == AUDIO_FU_CTRL_MUTE &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
      audio_control_cur_1_t result = {};
      result.bCur = speaker ? speaker_mute[channel]
                            : microphone_mute[channel];
      return tud_audio_buffer_and_schedule_control_xfer(
          rhport, request, &result, sizeof(result));
    }
    if (selector == AUDIO_FU_CTRL_VOLUME) {
      if (request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_2_t result = {};
        result.bCur = speaker ? speaker_volume[channel]
                              : microphone_volume[channel];
        return tud_audio_buffer_and_schedule_control_xfer(
            rhport, request, &result, sizeof(result));
      }
      if (request->bRequest == AUDIO_CS_REQ_RANGE) {
        if (speaker) {
          return tud_audio_buffer_and_schedule_control_xfer(
              rhport, request, &speaker_volume_range[channel],
              sizeof(speaker_volume_range[channel]));
        }
        return tud_audio_buffer_and_schedule_control_xfer(
            rhport, request, &microphone_volume_range[channel],
            sizeof(microphone_volume_range[channel]));
      }
    }
  }

  if (entity == kUacClockSource) {
    if (selector == AUDIO_CS_CTRL_SAM_FREQ) {
      if (request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_4_t result = {};
        result.bCur = speaker ? speaker_sample_rate : microphone_sample_rate;
        return tud_audio_buffer_and_schedule_control_xfer(
            rhport, request, &result, sizeof(result));
      }
      if (request->bRequest == AUDIO_CS_REQ_RANGE) {
        audio_control_range_4_n_t(1) range = {};
        range.wNumSubRanges = 1;
        range.subrange[0].bMin = kSampleRate;
        range.subrange[0].bMax = kSampleRate;
        range.subrange[0].bRes = 0;
        return tud_audio_buffer_and_schedule_control_xfer(
            rhport, request, &range, sizeof(range));
      }
    }
    if (selector == AUDIO_CS_CTRL_CLK_VALID &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
      audio_control_cur_1_t result = {};
      result.bCur = 1;
      return tud_audio_buffer_and_schedule_control_xfer(
          rhport, request, &result, sizeof(result));
    }
  }
  return false;
}

bool tud_audio_rx_done_pre_read_cb(uint8_t rhport, uint16_t bytes_received,
                                   uint8_t function_id, uint8_t endpoint_out,
                                   uint8_t alternate_setting) {
  (void)rhport;
  (void)bytes_received;
  (void)function_id;
  (void)endpoint_out;
  (void)alternate_setting;
  return true;
}

bool tud_audio_rx_done_post_read_cb(uint8_t rhport, uint16_t bytes_received,
                                    uint8_t function_id, uint8_t endpoint_out,
                                    uint8_t alternate_setting) {
  (void)rhport;
  (void)bytes_received;
  (void)function_id;
  (void)endpoint_out;
  (void)alternate_setting;
  if (codec_ready && function_id == kSpeakerFunctionId) {
    capture_usb_playback_locked();
    render_i2s_output();
    const uint32_t queued = playback_write_index - playback_read_index +
        tud_audio_n_available(kSpeakerFunctionId) / 4 +
        wm8960_i2s::queued_frames();
    // Sample once per USB packet; smooth packet/batch jitter over about 64 ms.
    playback_level_q8 +=
        (static_cast<int32_t>(queued * 256) - playback_level_q8) / 64;
    const int32_t error_q8 =
        static_cast<int32_t>(kPlaybackTargetFrames * 256) - playback_level_q8;
    const int32_t correction =
        std::max<int32_t>(-16384, std::min<int32_t>(16384, error_q8 / 8));
    playback_feedback = (48UL << 16) + correction;
    if (!tud_audio_n_fb_set(kSpeakerFunctionId, playback_feedback)) {
      ++playback_feedback_errors;
    }
  }
  return true;
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t function_id,
                                   uint8_t endpoint_in,
                                   uint8_t alternate_setting) {
  (void)rhport;
  (void)endpoint_in;
  (void)alternate_setting;
  if (function_id != kMicrophoneFunctionId) {
    return true;
  }

  // Capture follows USB packet deadlines, not foreground display/BT latency.
  // This is the sole reader of the RX DMA ring and producer of capture_ring.
  if (codec_ready) {
    capture_i2s_samples();
    i2s_receive_buffers = wm8960_i2s::received_frames() / 128;
  }
  static int16_t samples[49];
  std::memset(samples, 0, sizeof(samples));
  uint32_t read_index = capture_read_index;
  const uint32_t write_index = capture_write_index;
  const uint32_t queued = write_index - read_index;
  if (!capture_primed && queued >= 192) {
    capture_primed = true;
  }
  // Absorb small host/I2S clock differences without inserting zeros each ms.
  const uint8_t frames = queued > 288 ? 49 : queued < 96 ? 47 : 48;
  const uint32_t available = capture_primed ? queued : 0;
  const bool mute_for_tone = usb_audio_muted_for_keyer();
  if (capture_primed && available < frames) {
    capture_starved_packets = capture_starved_packets + 1;
    capture_primed = false;
  }
  for (uint8_t i = 0; i < frames; ++i) {
    capture_mute_level =
        advance_mute_ramp(capture_mute_level, mute_for_tone);
    if (capture_primed && i < available) {
      int32_t sample = capture_ring[read_index & (kCaptureRingFrames - 1)];
      sample = (sample * microphone_gain_q15) / 32767;
      sample = (sample * local_microphone_gain_q15) / 32767;
      samples[i] = apply_mute_ramp(static_cast<int16_t>(sample),
                                  capture_mute_level);
      ++read_index;
    }
  }
  capture_read_index = read_index;
  const uint16_t bytes = frames * sizeof(int16_t);
  return tud_audio_n_write(function_id, samples, bytes) == bytes;
}

void tud_audio_feedback_params_cb(uint8_t function_id, uint8_t alternate,
                                  audio_feedback_params_t *parameters) {
  (void)function_id;
  (void)alternate;
  parameters->method = AUDIO_FEEDBACK_METHOD_DISABLED;
  parameters->sample_freq = kSampleRate;
}

#endif
#endif
