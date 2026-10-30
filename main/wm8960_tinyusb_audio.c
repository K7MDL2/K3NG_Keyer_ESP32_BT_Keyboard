#if defined(ARDUINO_ARCH_RP2040)

#include "keyer_features_and_options_pico.h"

#if defined(FEATURE_WM8960_USB_AUDIO)

#include "wm8960_tinyusb_config.h"

#include <device/usbd.h>
#include <device/usbd_pvt.h>
#include <hardware/structs/watchdog.h>
#include <hardware/watchdog.h>

static bool trace_fifo_clear(tu_fifo_t *fifo) {
  if (!watchdog_caused_reboot()) {
    watchdog_hw->scratch[1] = 0x50;
  }
  bool result = tu_fifo_clear(fifo);
  if (!watchdog_caused_reboot()) {
    watchdog_hw->scratch[1] = 0x51;
  }
  return result;
}

static bool trace_iso_activate(uint8_t rhport,
                               tusb_desc_endpoint_t const *descriptor) {
  if (!watchdog_caused_reboot()) {
    watchdog_hw->scratch[1] = 0x60 | ((uint32_t)descriptor->bEndpointAddress << 8);
  }
  bool result = usbd_edpt_iso_activate(rhport, descriptor);
  if (!watchdog_caused_reboot()) {
    watchdog_hw->scratch[1] = 0x61;
  }
  return result;
}

static bool trace_control_status(uint8_t rhport,
                                 tusb_control_request_t const *request) {
  if (!watchdog_caused_reboot()) {
    watchdog_hw->scratch[1] = 0x70;
  }
  bool result = tud_control_status(rhport, request);
  if (!watchdog_caused_reboot()) {
    watchdog_hw->scratch[1] = 0x71;
  }
  return result;
}

#define tu_fifo_clear trace_fifo_clear
#define usbd_edpt_iso_activate trace_iso_activate
#define tud_control_status trace_control_status
#include <class/audio/audio_device.c>
#undef tu_fifo_clear
#undef usbd_edpt_iso_activate
#undef tud_control_status

static void traced_audio_reset(uint8_t rhport) {
  if (watchdog_caused_reboot()) {
    audiod_reset(rhport);
    return;
  }
  watchdog_hw->scratch[1] = 0x10;
  audiod_reset(rhport);
  watchdog_hw->scratch[1] = 0x11;
}

static uint16_t traced_audio_open(uint8_t rhport,
                                  tusb_desc_interface_t const *descriptor,
                                  uint16_t max_length) {
  if (watchdog_caused_reboot()) {
    return audiod_open(rhport, descriptor, max_length);
  }
  watchdog_hw->scratch[1] = 0x20;
  uint16_t result = audiod_open(rhport, descriptor, max_length);
  watchdog_hw->scratch[1] = 0x21;
  return result;
}

static bool traced_audio_control(uint8_t rhport, uint8_t stage,
                                 tusb_control_request_t const *request) {
  if (watchdog_caused_reboot()) {
    return audiod_control_xfer_cb(rhport, stage, request);
  }
  watchdog_hw->scratch[1] = 0x30 | ((uint32_t)stage << 8);
  watchdog_hw->scratch[2] =
      ((uint32_t)request->wIndex << 16) | request->wValue;
  watchdog_hw->scratch[3] =
      ((uint32_t)request->bmRequestType << 8) | request->bRequest;
  bool result = audiod_control_xfer_cb(rhport, stage, request);
  watchdog_hw->scratch[1] = 0x31;
  return result;
}

static bool traced_audio_transfer(uint8_t rhport, uint8_t endpoint,
                                  xfer_result_t result, uint32_t bytes) {
  if (watchdog_caused_reboot()) {
    return audiod_xfer_cb(rhport, endpoint, result, bytes);
  }
  watchdog_hw->scratch[1] = 0x40 | ((uint32_t)endpoint << 8);
  bool accepted = audiod_xfer_cb(rhport, endpoint, result, bytes);
  watchdog_hw->scratch[1] = 0x41;
  return accepted;
}

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
  static usbd_class_driver_t const audio_driver = {
      "AUDIO",
      audiod_init,
      audiod_deinit,
      traced_audio_reset,
      traced_audio_open,
      traced_audio_control,
      traced_audio_transfer,
      audiod_sof_isr,
  };
  *driver_count = 1;
  return &audio_driver;
}

#endif
#endif
