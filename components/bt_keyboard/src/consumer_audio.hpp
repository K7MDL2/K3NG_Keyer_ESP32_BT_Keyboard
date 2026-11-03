#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>

// Decode the three audio usages from array or bitmap reports, using their HID map.
inline bool decode_consumer_audio(const uint8_t *map, size_t map_size, uint8_t report_id,
                                  const uint8_t *data, size_t data_size, uint8_t &pressed) {
  struct Globals { uint32_t page = 0, size = 0, count = 0, id = 0; } globals, stack[8];
  size_t depth = 0;
  uint32_t usages[32] = {};
  size_t usage_count = 0;
  uint32_t minimum = 0, maximum = 0;
  bool has_range = false, recognized = false;
  size_t bit_offset = 0;
  pressed = 0;
  auto audio_bit = [](uint32_t usage) -> uint8_t {
    return usage == 0x000C00E2 ? 1 : usage == 0x000C00E9 ? 2 : usage == 0x000C00EA ? 4 : 0;
  };
  for (size_t pos = 0; pos < map_size;) {
    const uint8_t prefix = map[pos++];
    if (prefix == 0xFE) {
      if (map_size - pos < 2) return false;
      const size_t length = map[pos];
      pos += 2;
      if (length > map_size - pos) return false;
      pos += length;
      continue;
    }
    const size_t bytes = (prefix & 3) == 3 ? 4 : prefix & 3;
    if (bytes > map_size - pos) return false;
    uint32_t value = 0;
    for (size_t n = 0; n < bytes; ++n) value |= uint32_t(map[pos++]) << (8 * n);
    const uint8_t type = (prefix >> 2) & 3, tag = prefix >> 4;
    if (type == 1) {
      switch (tag) {
        case 0: globals.page = value; break;
        case 7: globals.size = value; break;
        case 8: globals.id = value; break;
        case 9: globals.count = value; break;
        case 10:
          if (depth == 8) return false;
          stack[depth++] = globals;
          break;
        case 11:
          if (!depth) return false;
          globals = stack[--depth];
          break;
      }
    } else if (type == 2) {
      const uint32_t usage = bytes == 4 ? value : (globals.page << 16) | value;
      if (tag == 0) {
        if (usage_count == 32) return false;
        usages[usage_count++] = usage;
      } else if (tag == 1) {
        minimum = usage;
        has_range = true;
      } else if (tag == 2) {
        maximum = usage;
      }
    } else if (type == 0) {
      if (tag == 8 && globals.id == report_id) {
        if (globals.size > 32 || globals.count > data_size * 8 ||
            uint64_t(bit_offset) + uint64_t(globals.size) * globals.count > data_size * 8) return false;
        if (!(value & 1)) {
          for (uint32_t field = 0; field < globals.count; ++field) {
            uint32_t sample = 0;
            for (uint32_t bit = 0; bit < globals.size; ++bit) {
              const size_t offset = bit_offset + field * globals.size + bit;
              sample |= uint32_t((data[offset / 8] >> (offset % 8)) & 1) << bit;
            }
            if (value & 2) {
              const uint32_t usage = field < usage_count ? usages[field] :
                  has_range && maximum >= minimum && field <= maximum - minimum ? minimum + field :
                  usage_count ? usages[usage_count - 1] : 0;
              const uint8_t mask = audio_bit(usage);
              recognized |= mask != 0;
              if (sample) pressed |= mask;
            } else {
              // Array samples are usage IDs, not bit positions.
              for (uint32_t usage : {0x000C00E2u, 0x000C00E9u, 0x000C00EAu}) {
                bool declared = has_range && usage >= minimum && usage <= maximum;
                for (size_t n = 0; n < usage_count; ++n) declared |= usages[n] == usage;
                recognized |= declared;
                if (declared && sample == (usage & 0xFFFF)) pressed |= audio_bit(usage);
              }
            }
          }
        }
        bit_offset += globals.size * globals.count;
      }
      usage_count = 0;
      minimum = maximum = 0;
      has_range = false;
    }
  }
  return recognized;
}
