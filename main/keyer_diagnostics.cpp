#include "keyer_diagnostics.h"

const char *keyer_foreground_stage_name(uint8_t stage) {
  switch (stage) {
    case 9: return "keyer/buttons";
    case 10: return "serial";
    case 11: return "touch";
    case 12: return "Bluetooth keyboard";
    case 13: return "keyboard/GPS";
    case 14: return "settings/memories";
    case 15: return "display";
    case 16: return "other foreground";
    case 17: return "BOOTSEL";
    case 18: return "loop/audio";
    case 19: return "send buffer/PTT";
    case 20: return "potentiometer/encoders";
    default: return "none";
  }
}
