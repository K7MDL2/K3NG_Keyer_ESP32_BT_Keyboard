#if defined(ARDUINO_ARCH_RP2040)
#include "keyer_hardware.h"
#include "keyer_features_and_options_pico.h"
#if defined(FEATURE_WM8960_USB_AUDIO) && defined(FEATURE_ROTARY_ENCODER)
#include "encoder_settings.h"
#include "wm8960_audio.h"
#include <LittleFS.h>

extern uint8_t _FS_start, _FS_end;

namespace {
constexpr const char *kSettingsPath = "/keyer-menu.bin";
constexpr const char *kTemporaryPath = "/keyer-menu.tmp";

class PauseAudioForFlash {
 public:
  PauseAudioForFlash() { wm8960_audio_pause_for_flash(); }
  ~PauseAudioForFlash() { wm8960_audio_resume_after_flash(); }
};
}

bool encoder_settings_begin() {
  if (reinterpret_cast<uintptr_t>(&_FS_end) - reinterpret_cast<uintptr_t>(&_FS_start) < 65536) {
    wm8960_audio_debug_port().println("Error: menu persistence requires at least 64 KB filesystem");
    return false;
  }
  PauseAudioForFlash pause;
  if (!LittleFS.setConfig(LittleFSConfig(false))) return false;
  if (LittleFS.begin()) return true;
  // Initialize only a completely erased partition, never a corrupt/existing filesystem.
  const uint8_t *first = &_FS_start;
  const size_t size = reinterpret_cast<uintptr_t>(&_FS_end) - reinterpret_cast<uintptr_t>(first);
  for (size_t i = 0; i < size; ++i) {
    if (first[i] != 0xFF) {
      wm8960_audio_debug_port().println("Error: menu filesystem mount failed; nonblank partition not formatted");
      return false;
    }
  }
  wm8960_audio_debug_port().println("Initializing blank menu filesystem");
  return LittleFS.format() && LittleFS.begin();
}

bool encoder_settings_load(EncoderSettings &settings, bool &found) {
  found = LittleFS.exists(kSettingsPath);
  if (!found) return true;
  File file = LittleFS.open(kSettingsPath, "r");
  if (!file) return false;
  if (file.size() == LegacyEncoderSettingsRecord{}.size()) {
    LegacyEncoderSettingsRecord legacy{};
    if (file.read(legacy.data(), legacy.size()) != static_cast<int>(legacy.size())) return false;
    return decode_encoder_settings(legacy, settings);
  }
  EncoderSettingsRecord record{};
  if (file.size() != record.size() ||
      file.read(record.data(), record.size()) != static_cast<int>(record.size())) return false;
  return decode_encoder_settings(record, settings);
}

bool encoder_settings_save(const EncoderSettings &settings) {
  PauseAudioForFlash pause;
  const EncoderSettingsRecord record = encode_encoder_settings(settings);
  File file = LittleFS.open(kTemporaryPath, "w");
  if (!file) return false;
  const bool written = file.write(record.data(), record.size()) == record.size();
  file.flush();
  file.close();
  if (!written) return false;
  // Re-read after close to catch write/sync failures before atomic replacement.
  File verify = LittleFS.open(kTemporaryPath, "r");
  EncoderSettingsRecord readback{};
  if (!verify || verify.size() != record.size() ||
      verify.read(readback.data(), readback.size()) != static_cast<int>(readback.size()) ||
      readback != record) return false;
  verify.close();
  return LittleFS.rename(kTemporaryPath, kSettingsPath);
}
#endif
#endif
