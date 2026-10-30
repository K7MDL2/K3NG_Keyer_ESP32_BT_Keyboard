#if defined(ARDUINO_ARCH_RP2040)
#include "keyer_features_and_options_pico.h"

#if defined(FEATURE_WM8960_USB_AUDIO)
#include "wm8960_tinyusb_config.h"

// Replace the core's prebuilt single-port CDC driver with a two-port instance.
#define tud_cdc_line_coding_cb keyer_cdc_line_coding_cb
#define tud_cdc_line_state_cb keyer_cdc_line_state_cb
#include <class/cdc/cdc_device.c>
#undef tud_cdc_line_coding_cb
#undef tud_cdc_line_state_cb

extern void tud_cdc_line_coding_cb(uint8_t itf,
                                  cdc_line_coding_t const *coding);
extern void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts);

void keyer_cdc_line_coding_cb(uint8_t itf,
                             cdc_line_coding_t const *coding) {
  if (itf == 0) {
    // Keep host coding intact in TinyUSB, but hide the BOOTSEL trigger from
    // SerialUSB. USB CDC does not use this baud rate for data transport.
    cdc_line_coding_t core_coding = *coding;
    if (core_coding.bit_rate == 1200) {
      core_coding.bit_rate = 115200;
    }
    tud_cdc_line_coding_cb(itf, &core_coding);
  }
}

void keyer_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  // Arduino's SerialUSB callbacks otherwise treat CDC2 as the primary port.
  if (itf == 0) {
    tud_cdc_line_state_cb(itf, dtr, rts);
  }
}
#endif
#endif
