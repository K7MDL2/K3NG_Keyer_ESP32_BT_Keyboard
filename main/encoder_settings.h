#ifndef ENCODER_SETTINGS_H
#define ENCODER_SETTINGS_H

#include <array>
#include <cstdint>

using EncoderSettings = std::array<uint8_t, 36>;
using EncoderSettingsRecord = std::array<uint8_t, 44>;
using LegacyEncoderSettingsRecord = std::array<uint8_t, 42>;

inline uint32_t encoder_settings_checksum(const uint8_t *bytes, unsigned size) {
  uint32_t hash = 2166136261u;
  for (unsigned i = 0; i < size; ++i) hash = (hash ^ bytes[i]) * 16777619u;
  return hash;
}

inline EncoderSettingsRecord encode_encoder_settings(const EncoderSettings &settings) {
  EncoderSettingsRecord record{};
  record[0] = 'K'; record[1] = 'M'; record[2] = 'S'; record[3] = 2;
  for (unsigned i = 0; i < settings.size(); ++i) record[i + 4] = settings[i];
  const uint32_t checksum = encoder_settings_checksum(record.data(), 40);
  for (unsigned i = 0; i < 4; ++i) record[40+i] = checksum >> (8*i);
  return record;
}

inline bool decode_encoder_settings(const EncoderSettingsRecord &record, EncoderSettings &settings) {
  if (record[0] != 'K' || record[1] != 'M' || record[2] != 'S' || record[3] != 2) return false;
  uint32_t checksum = 0;
  for (unsigned i = 0; i < 4; ++i) checksum |= uint32_t(record[40+i]) << (8*i);
  if (checksum != encoder_settings_checksum(record.data(), 40)) return false;
  for (unsigned i = 0; i < settings.size(); ++i) settings[i] = record[i + 4];
  return true;
}

inline bool decode_encoder_settings(const LegacyEncoderSettingsRecord &record, EncoderSettings &settings) {
  if (record[0] != 'K' || record[1] != 'M' || record[2] != 'S' || record[3] != 1) return false;
  uint32_t checksum = 0;
  for (unsigned i = 0; i < 4; ++i) checksum |= uint32_t(record[38+i]) << (8*i);
  if (checksum != encoder_settings_checksum(record.data(), 38)) return false;
  const uint8_t *old = record.data()+4;
  for (unsigned i = 0; i < 10; ++i) {
    if (old[i] >= 4 || old[18+i] > 1) return false;
    settings[i] = old[i];
    settings[19+i] = old[18+i];
  }
  settings[10] = 0; // New Local RX control belongs to encoder 1.
  settings[29] = 1;
  for (unsigned i = 0; i < 4; ++i) {
    const unsigned selected = old[10+i];
    if (selected > 10 || !old[14+i]) return false;
    if (selected < 10 && old[selected] != i) return false;
    if (selected == 10)
      for (unsigned function = 0; function < 10; ++function)
        if (old[function] == i) return false;
    settings[11+i] = selected == 10 ? (i == 0 ? 10 : 11) : selected;
    settings[15+i] = old[14+i];
  }
  for (unsigned i = 0; i < 6; ++i) settings[30+i] = old[28+i];
  return true;
}

bool encoder_settings_begin();
bool encoder_settings_load(EncoderSettings &settings, bool &found);
bool encoder_settings_save(const EncoderSettings &settings);

#endif
