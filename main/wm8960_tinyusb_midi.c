#if defined(ARDUINO_ARCH_RP2040)
#include "keyer_features_and_options_pico.h"

#if defined(FEATURE_WM8960_USB_AUDIO)
#include "wm8960_tinyusb_config.h"

// Replace Arduino-Pico's weak MIDI stubs so the advertised interface can open.
#include <class/midi/midi_device.c>
#endif
#endif
