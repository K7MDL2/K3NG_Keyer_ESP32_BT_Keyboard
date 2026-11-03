#if defined(ARDUINO_ARCH_RP2040)
#include "keyer_features_and_options_pico.h"

#if defined(FEATURE_WM8960_USB_AUDIO)
#include "wm8960_tinyusb_config.h"

#define dcd_edpt_iso_activate wm8960_unpatched_iso_activate
#include <portable/raspberrypi/rp2040/dcd_rp2040.c>
#undef dcd_edpt_iso_activate

// Same ISO reactivation backport used by the SDR project's build overlay.
static void wm8960_abort_iso_transfer(struct hw_endpoint *ep) {
  const uint8_t direction = tu_edpt_dir(ep->ep_addr);
  const uint8_t endpoint = tu_edpt_number(ep->ep_addr);
  const uint32_t abort_mask =
      TU_BIT((endpoint << 1) | (direction ? 0 : 1));

  if (rp2040_chip_version() >= 2) {
    usb_hw_set->abort = abort_mask;
    while ((usb_hw->abort_done & abort_mask) != abort_mask) {}
  }

  uint32_t buffer_control = USB_BUF_CTRL_SEL;
  if (ep->next_pid) {
    buffer_control |= USB_BUF_CTRL_DATA1_PID;
  }
  _hw_endpoint_buffer_control_set_value32(ep, buffer_control);
  hw_endpoint_reset_transfer(ep);

  if (rp2040_chip_version() >= 2) {
    usb_hw_clear->abort_done = abort_mask;
    usb_hw_clear->abort = abort_mask;
  }
}

bool dcd_edpt_iso_activate(uint8_t rhport,
                           tusb_desc_endpoint_t const *descriptor) {
  (void)rhport;
  struct hw_endpoint *ep =
      hw_endpoint_get_by_addr(descriptor->bEndpointAddress);
  TU_ASSERT(ep->hw_data_buf != NULL);
  if (ep->active) {
    wm8960_abort_iso_transfer(ep);
  }
  ep->wMaxPacketSize = descriptor->wMaxPacketSize;
  hw_endpoint_enable(ep);
  return true;
}
#endif
#endif
