| Support Targets | ![ESP32-WROOM32][esp32] | ![Pico2W][Pico2W] | ![alt text][Has_Precompiled_Firmware_Images] |
| --- | --- | --- | --- |

| Dev Environment Used | ![alt text][ESP-IDF6] | ![Arduino IDE][Arduino]|
| --- | --- | --- | --- |

| BT Keyboards Tested | ![alt text][K380]|![alt text][K380s] | ![Rii i8+ mini][Rii_mini] |
| --- | --- | --- | --- |

[Pico2W]: https://img.shields.io/badge/Pico2W-green "ESP32"
[ESP32]: https://img.shields.io/badge/ESP32--WROOM-orange "ESP32-WROOM"
[ESP-IDF6]: https://img.shields.io/badge/ESP--IDF--v6.0.1-cyan "ESP-IDF v6.0.1"
[Arduino]: https://img.shields.io/badge/Arduino-cyan "Arduino v2.3.7"
[Has_Precompiled_Firmware_Images]: https://img.shields.io/badge/Has_Precompiled_Firmware_Images-purple "Precompiled_Images"
[K380]: https://img.shields.io/badge/K380-violet "K380"
[K380s]: https://img.shields.io/badge/K380s-violet "K380s"
[Rii_mini]: https://img.shields.io/badge/Rii_i8+_mini-violet "Rii i8+"


# K3NG CW Keyer (Modified for ESP32 and Pico2-W)

Based on a 2022 version modifed to run on ESP32-WROOM32 and Pico2W.  

Select relevant updates from 2022 to Dec 2025 have been applied.  Added BT keyboards, TFT, Touch, LCD displays, GPS, and Compass. The ESP32 build uses ESP-IDF for full Bluetooth keyboard support. Arduino IDE can compile for ESP32, but Bluetooth keyboard operation is not supported for all keyboard models there. Pico2W builds use Arduino.

To build the ESP32 under ESP-IDF see https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Building-the-Project-with-ESP%E2%80%90IDF

To build under Arduino IDE see https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Building-the-project-with-Arduino-IDE.  For the Pico I post a UF2 image file, and image files for the ESP32 for each model display for those who do not want to try to compile their own.   

Under Arduino, for ESP32 and Pico only,  I automatically detect the board type and set the proper #define for that board, no need to edit the keyere-hardware.h file.  If you have a different display, or no display, update the settings in Feature and Options header file.  TFT_eSPI library is used for both ESP32 and Pico.  Your TFT display needs to be one of the types supported in TFT_eSPI's UserSetup.h file.  I have tested displays with both the GT911 and XPT2046 touch controllers.  The XPT2046 has built in support in the TFT_eSPI libary.  GT911 is an i2c controller and uses a separate library.  It should be easy to substitute other touch controllers, just need a 'pressed' event (polled) with x and y values.

_____________________________________________
## From the original K3NG site:

The K3NG Keyer is an open source Arduino based CW (Morse Code) keyer with a lot of features and flexibility, rivaling commercial keyers which often cost significantly more. The code can be used with a full blown Arduino board or an AVR microcontroller chip can be programmed and used directly in a circuit. This keyer is suitable as a standalone keyer or for use permanently installed inside a rig, especially homebrew QRP rigs. It’s open source code so you can fully customize it to fit your needs and also perhaps learn from it or find coding ideas for other projects.

> [!NOTE]
>Original K3NG CW Keyer here: https://github.com/k3ng/k3ng_cw_keyer
> 
>Documentation is located here: https://github.com/k3ng/k3ng_cw_keyer/wiki
_____________________________________________

## This fork on a ESP32-WROOM32 and Pico2W adds:
 - BT keyboards (Select BLE and BT Classic models)
 - A few different type TFT displays with 320x170, 320,240, and 480x320 resolutions
 - Touch buttons on touch enabled displays
 - Status Bar with Grid, call, WPM rate, and several status icons including T/R status.
 - MCP23017 I2C port expander to add IO to the IO-starved TFT display boards.  The Paddles and Straight key are on PA0-2 pins.
 - Using interrupts for all paddle and straight key IO pin, both the port expander and local GPIO pins.
 - Serial GPS can udpate time and grid square to 8 places, and populate a memory with 4 or 6 digits.
 - I2C compass heading values can display in a popup window to help point microwave antennas.  Can configure your declination.
 - Backlight timeout with enable/disable to save battery. Configurable timeout. TFT displays are around 270ma, 100ma with backlight off. About 10ma less without BT keyboard feature.
 - User assignable buttons and number of button rows.
 - For ESP32, optionally run GPS, BT, Touch, and Main loop in task.  All but Main Loop can run on either Core 0 or Core 1 (as a group move).  Lets BT connections and GPS time to run uninterrupted, even during 13WPM and 50WPM keying rates.  For Pico GPS and BT connect/reconnect can run on Core 1 (no task requried).  When BT is on Core1 it allows connect and reconnect in the background.
 - Compiles under ESP-IDF v6.1.0 as of September 2026.  Addresses several deprecated issues in some libraries.  Review the Change Notes Wiki Page for some manual managed component library fixes required for v6.  Hopefully Espressif will catch these libs up.
 - FEATURE_BUTTONS and COMMAND_MODE now working.
 - Sept 2026 have BT Classic and BLE keyboards pairing with receonnection on both ESP32 and Pico2W. The BT Classic keyboard, Logitech K380, pairs with a PIN teh first time, then after, pairs to receonnct, no PIN required so it is reasonably fast.

Check out the latest changes at the Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Change-Notes

> [!NOTE]
> Arduino IDE Users with ESP32 see https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Building-the-project-with-Arduino-IDE
>

See the Wiki Pages for more information about parts supported and configuration. https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki

_____________________________________________

## Project Info

This is a 3.5" 480x320 TFT with a single panel of 16 keys.  If you press the Mem key, a blue message window pops up diplaying the contents of each memory.  You can use Insert Memory macro '\Ix' to insert memories (1-9 only).  For example I have memories 7 and 8 programmed with my callsign and grid square.

<img width="627" height="385" alt="image" src="https://github.com/user-attachments/assets/da19e558-d734-4327-8b30-68130b4608d2" />

This is a 3.5" 320x480 TFT (Pioc2W) with 4 rows of 4 keys (not counting the Fx key to change rows).  Has the identical 16 key assignments.

![320x480-K7MDL-Feb-17-2026](https://github.com/user-attachments/assets/cdd84d6c-8700-46c9-adaf-570311156e7a)

This is a 3.2" 320x240 TFT (ESP32) with 4 rows of 4 keys (not counting the Fx key to change rows).  Has the identical 16 key assignments.

<img width="1146" height="610" alt="image" src="https://github.com/user-attachments/assets/b4512298-544b-4bc3-94d5-6bd83fdc7d14" />

Also seen in the above picture is a 16 port expander (MCP23017) on a I2C bus.  The paddles and straight key are on PA0-2.

The ESP and PICO look and act identical with some minor differences around BT keyboard connection behaviors.

*************************************************************************

3 years ago (2022) SP5IOU modified the K3NG Keyer Arduino code to support an ESP32.  This repository is forked from his repository at https://github.com/aimeiz/k3ng_cw_keyer-master_2022.  After changing the pin assignments it worked on my ESP32-WROOM-32 dev board.  I used the board described in this Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/CPU-Module

I then integrated a BT Keyboard library from https://github.com/turgu1/bt-keyboard.  It is a bit different in that the ESP32 is a HID host connecting to a BT keyboard for input.  Most examples just convert a USB or PS2 keyboard to BT to connect to a PC.  Later I added several types of TFT displays and touch buttons for certain models.  Finally I added support for the PicoW and Pico2W. Only tested on Pico2-W (RP2350A and RP2350B).

ESP32: I am using ESP-IDF to compile.  Arduino-esp32 is added as a component.  I first tried Arduino IDE but I was not able to get a BT classic keyboard (Logitech K380) to fully connect after it was discovered.  My BLE Rii i8+ keyboard worked fine though.  My BT_Keyboard test programs behaved the same.  I ported it to esp-idf and BT classic works proper as does the Rii and the K380s BLE keyboard. You can compile this under Arduino IDE but see this page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Building-the-project-with-Arduino-IDE for current functionality restrictions.

Pico: Using Arduino IDE to compile with the arduino-pico board package.  Supports BT Classic or BLE based on a compile time switch.  BLE connects and reconnects, No PIN required.  BT Classic connects with a PIN adn but requires pushing the keyboard Pair button each connection aftawrds, no PIN required though.

The BT keyboard code translates BT key codes to match the PS2 keycodes and calls into the slightly modified PS2 keyboard function.  See the K3NG docs for USB\PS2 Keyboard commands. I have a copy of the BT keyboard commands on this Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/BT-Keyboard-Key-Assignments.   Not all keys on a PS2/USB keyboard are present on these compact BT keyboards.   I may alter some of the key assignments over time.

Tested with BLE K380s and Rthe ii i8+ mini keyboards, and the Logitech K380, which uses BT classic.

I have manually merged the most relevant updates from the orignal k3NG repo add since 2022.  Since this version is currently compiled under esp-idf framework, it is not likely to be accepted into the main repo.  You can compile for ESP32 under Arduino but not every (or any) BT keyboards work yet per the above notes.  Pico is compiled with Arduino.  Same code for both.  In fact you can compile them a the same time.

I have several WIKI pages to show how I set up the ESP-IDF options.  The TFT_eSPI, BT_keyboard, and other required components (aka libraries) are included in tis repo so there should be no need to find and download libraries and components unlike in Arduino.  For Arduino builds you will need to copy some of the libraries to your standard Arduino/libraries path.  See the Wiki Notes for details.

I provide several precompiled firmware files for your compatible CPU board using either of 2 tools so you do not have to compile the code, just upload the binary files and run it. There are Wiki pages about hardware, other info, and how to upload the firmware: 

https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki

## Pico 2 W WM8960 audio

The optional WM8960 path is Pico-only. It adds 48 kHz,
16-bit stereo USB speaker playback, mono USB microphone capture, and routes
K3NG sidetone/beeps through the codec. ESP32-WROOM32 audio behavior is unchanged.

### Saved audio levels and controls

The dormant `FEATURE_SINEWAVE_SIDETONE` DAC/timer implementation was removed.
Its only stored setting, `configuration.sidetone_volume`, is reused as WM8960
sidetone volume in percent. The old initialization set it to 255, the midpoint
of the former 10..500 limits; those obsolete limits are removed.
`configuration.hz_sidetone` still controls frequency.
New `usb_play_volume`, `usb_mic_volume`, `sidetone_muted`, `usb_play_muted`, and
`usb_mic_muted` fields are saved in the same EEPROM configuration structure.
The MIDI port also adds the saved `midi_ptt_mode` field, defaulting to off.
All levels default to 100%, with mute off. Sidetone 100% preserves the previous
8192-peak tone amplitude; USB 100% preserves the previous USB gain. These are
device-side linear-amplitude trims, combined with (not replacing) host USB
mute/volume settings. Microphone analogue PGA gain is unchanged.
Headphone and speaker analogue output volumes are both 0x78 (-1 dB),
8 dB above the earlier 0x70 setting; right-channel writes retain the paired
volume-update bit (0x178).

EEPROM magic is now 43 for Pico and 46 for ESP32 because the shared structure
changed. **The first boot resets saved settings and initializes CW memories.**
Bluetooth pairing/reconnection settings may need to be established again.
Changes use the normal deferred EEPROM save when idle; allow at least the
configured `eeprom_write_time_ms` before removing power.

With `FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT` enabled, the Pico
WM8960 build runs the terminal on **USB CDC2**, while WinKey stays on USB CDC1. Non-audio builds retain the UART `Serial1` mapping.
Audio/timing diagnostics also use CDC2 when that terminal flag is enabled,
so those messages do not contaminate the WinKey protocol on CDC1.
`DEBUG_WM8960_AUDIO` in `main/keyer_debug.h` enables routine codec/PIO startup
messages and serial-connect audio/timing dumps. Comment it out to suppress
those messages and the diagnostic LRCLK sampling. Initialization/I2C errors
and retained crash reporting remain enabled.
Open the terminal at 115200 baud with DTR enabled. Native USB does not use the
legacy WinKey startup-byte-discard option, which could block both interfaces.
The sketch-local CDC driver disables Arduino-Pico's 1200-baud/DTR BOOTSEL
reset for this composite firmware. WinKey can open/close at 1200 baud without
entering the flashing drive. Host line coding is retained by TinyUSB, primary
DTR/RTS handling is preserved, and CDC2 controls cannot change primary state.
Automatic 1200-baud-touch uploading is unavailable; use physical BOOTSEL to
flash from Arduino IDE or copy a UF2. No installed Arduino core patch is needed.

Extended terminal commands start with `\:` and end with Enter:

| Command | Action |
|---|---|
| `\:AUDIO` | Show all saved device levels and mute states |
| `\:USBPLAY 65` | Set USB playback to 65% |
| `\:USBMIC 80` | Set USB microphone level to 80% |
| `\:SIDETONE 50` | Set sidetone to 50% |
| `\:USBPLAY MUTE` / `\:USBPLAY UNMUTE` / `\:USBPLAY TOGGLE` | Change playback mute without losing its level |
| `\:USBMIC +` / `\:SIDETONE -` | Adjust the selected level by approximately 1 dB |

All three target commands accept a level from 0 to 100, `+`, `-`, `MUTE`,
`UNMUTE`, or `TOGGLE`; no argument shows status. Setting/adjusting a level
unmutes that target. Zero level is silent. Invalid input reports an error
without changing settings. `\S` also includes audio status; `\?` lists commands.

Bluetooth consumer/media keys use the same controls:

| Prefix | Mute / Volume+ / Volume- target |
|---|---|
| None | USB playback |
| Left or right Shift | WM8960 sidetone |
| Left or right Ctrl | USB microphone |

Mute toggles the selected path; Volume+/Volume- use approximately 1 dB
audio-taper steps and unmute, as do terminal `+`/`-` commands. Each step
multiplies linear amplitude by 1.122 (up) or 0.891 (down), rounded to the
saved integer percentage. Near silence, a minimum 1% change ensures every
step moves; 0% is silence and 100% is the upper limit. For example, decreasing
from 100% gives 89%, 79%, 70%, 62%, then 55%. Numeric commands still set
linear 0..100% directly, and the EEPROM layout/version is unchanged.
Ctrl+Shift or Alt/Meta combinations are ignored. Consumer releases do not
repeat the action or enter the Morse text buffer. The keyboard must actually
send consumer usages 0xE2/0xE9/0xEA while modifiers are held (Fn behavior is
keyboard-specific). Muting sidetone does not disable transmitter keying or
the existing seven-dit USB keyer-priority hold and 5 ms fades.
Hardware tests passed all nine consumer-key combinations, terminal level/mute
commands and invalid-input rejection, saved levels and mute states across
power cycles, and unmute recovery of all three audio paths. The final firmware
also passed CDC2 diagnostic/status queries and a quiet CDC1 open followed by
the expected 15-byte WinKey settings reply.

Before enabling it, wire a WM8960 module with an onboard or external I2S MCLK
source and verify the pin assignments in `main/keyer_pin_settings_pico.h`.
The Waveshare WM8960 Audio Board has a 24 MHz onboard oscillator. Set
`WM8960_MCLK_HZ` to `24000000UL` for this board; the codec PLL generates the
12.288 MHz internal clock needed for 48 kHz audio. A module supplied with
12.288 MHz MCLK can instead use `12288000UL` to bypass the PLL.
The current display profile explicitly sets `WM8960_USE_PLL` to `1`.
A direct-MCLK comparison with the SDR firmware did not restore audio.
Direct 24 MHz MCLK is not the nominal 256-times-48-kHz clock and is not
a validated production clock configuration.
Codec register writes use the SDR driver's bounded recovery pattern: report
the error, release the I2C bus with up to nine clock pulses and a STOP,
reinitialize the controller, and retry once. A second failure aborts audio
initialization; the serial status retains the last register/error even if
recovery succeeds.
The WM8960 transport uses the SDR project's TX and left-channel RX PIO
programs with aligned 2048-frame circular buffers and RP2350 endless DMA.
It does not use Arduino I2S/AudioBufferManager, linked DMA buffer lists, or
audio DMA interrupts. PIOProgram allocates state machines alongside existing
Arduino users. The TX divider is calculated from the existing system clock
rather than changing it after USB/Bluetooth startup.
The `.pio.h` headers are checked in, so Arduino IDE needs no CMake or assembler
step. To regenerate them using Arduino-Pico's installed `pioasm`, run
`pioasm -o c-sdk main/wm8960_i2s_tx.pio main/wm8960_i2s_tx.pio.h`
and the equivalent command for `wm8960_i2s_rx`.
The bundled TFT_eSPI RP2 `dmaWait()` checks `DMA_Enabled` before waiting,
matching `dmaBusy()`. Without that guard, touch reads with display DMA disabled
can wait forever on default channel 0 after audio claims it for endless DMA.
Arduino IDE may use a separate installed TFT_eSPI rather than this bundled
copy. Its `Processors/TFT_eSPI_RP2040.c` must have the same
`if (!DMA_Enabled) return;` guard at the start of `TFT_eSPI::dmaWait()`.
An unpatched copy can cause a startup watchdog at `WD S:11` (touch handling).
Hardware testing with the PIO transport and this guard passed Bluetooth/paddle
operation up to 49 WPM and touch input. USB speaker and microphone now carry
recognizable audio, but both still have audible fuzzy/noisy distortion.
Subsequent packet-deadline servicing tests produced clean PC speaker playback
with stable keyer muting; microphone quality improved, with possible residual
speech-start/stop transients still under investigation. Longer-duration
stability remains to be validated. The revised codec startup sequence below
passed three consecutive user-tested cold starts with audio and controls working.
USB playback feedback tracks the combined TinyUSB, PCM, and DMA output queues,
not TinyUSB's FIFO alone. A 64-packet smoothing filter targets 768 queued frames,
adjusting the requested host rate by at most 0.25 sample per millisecond around
48 samples/ms. Playback primes before consuming a new stream.
TinyUSB's event hook immediately pends Arduino-Pico's existing USB task IRQ;
the core's normal 1 ms timer remains a fallback. This reduces ISO endpoint
rearming latency without calling the task inside the USB hardware interrupt
or bypassing the core USB mutex.
In hardware retesting, combined-queue feedback alone did not remove puffing.
Adding immediate USB-task wakeup produced clean spoken-video playback with
zero missing playback frames and zero microphone short packets; the filtered
playback queue stayed near its 768-frame target. BT/paddles and tone muting
remained stable during that test.
Capture drains larger I2S batches, primes a four-millisecond cushion, and
supplies 47/48/49-frame USB packets according to queue occupancy. Serial status
reports playback missing frames and capture short packets to distinguish
buffer starvation from analogue clipping or I2S signal problems.
RX DMA ring draining runs exclusively in the USB microphone packet callback,
not the foreground keyer loop, so display/Bluetooth delays do not starve
capture packets. No blocking I2C, serial, or display operations run in that
callback. Speaker packet callbacks also drain playback and render output,
so incoming USB audio can keep the DMA ring fed during foreground delays.
Foreground rendering remains available for standalone keyer tones. Both
renderer paths are serialized by the core USB mutex.
The microphone input PGA remains at +6.75 dB (register value 0x20).
ADC digital gain remains 0 dB.
While keying, PC playback is suppressed in the codec output
and microphone packets sent to the PC contain silence. Both USB streams
continue consuming samples. Muting spans letter and word gaps and releases
after seven dit lengths without another tone, using the speed at tone start
(including command-mode speed and QRSS dit duration). Each new tone extends
the mute interval. Both USB directions fade out over 5 ms when muting begins
and fade back in over 5 ms after the hold expires. A new tone during fade-in
smoothly reverses the fade. Local keyer tones are not attenuated; PC playback
briefly mixes with the tone during fade-out, with saturation to prevent clipping
wraparound. Host mute/volume settings are not changed.
For timing diagnosis, the last 128 software key-state transitions are retained
without serial output during keying. Reopening the diagnostic serial port
(CDC2 when the secondary terminal is enabled, otherwise CDC1) prints
`[TIMING] time_us,set_wpm,element_dits,down` records. Hold one paddle to generate
continuous dits or dahs, release it, then read the records. For consecutive dits,
actual element WPM is 2,400,000 divided by the average down-to-down period in
microseconds (4,800,000 for dahs). This includes key-up spacing, but not
letter/word spacing; a PARIS test is needed for overall text WPM.
These timestamps measure software keying, not physical GPIO or codec edges.
Continuous-paddle dit measurements with audio running: set 20 WPM measured
19.902 WPM (60.108 ms down, 60.486 ms up); set 49 WPM measured 49.417 WPM
(24.116 ms down, 24.450 ms up). Each recording contained 64 dits.
A separate set-6-WPM test measured 5.993 WPM across 29 complete dit cycles
(200.105 ms down, 200.338 ms up), with codec audio initialization failed
during that test.
Keyboard/PARIS text rate and physical output timing remain unmeasured.
Pico DOUT connects to `I2S_DAC` (header pin 13), and Pico DIN connects to
`I2S_ADC` (header pin 14). The MCLK_RX/TX jumper routes the oscillator clock
to the header; it does not select the I2S data direction.
Use the display-profile pin definitions as the source of truth for your wiring.
Codec control transfers use 100 kHz I2C, including after bus recovery, to
improve startup signalling margin. DAC unmute completes before the PIO I2S
clocks start; the DMA output ring is initialized to silence. Each control
transfer has an explicit 5 ms timeout. Recovery uses open-drain
clock pulses, waits up to 500 us for SCL to rise, and checks that both lines
are released before retrying once. Failures remain explicit; reconnecting
serial reports the last failing register, transfer duration, and bus levels.
Reducing I2C speed alone did not eliminate the unmute failure. Completing
unmute before I2S startup then passed upload startup and three cold starts;
the final serial check reported audio ready with no recorded I2C error.
The I2C controller is selected from the codec SDA/SCL pins. GPIO20/21 use
`Wire` (I2C0) and can share the expander/compass bus. If sharing an I2C
controller, use the same SDA/SCL pair as the other devices.
I2S LRCLK must be BCLK+1, and all four I2S pins must fit one PIO GPIO window
(GPIO0-31 or, on RP2350B, GPIO16-47). Avoid conflicts with display, touch,
keyer, and other hardware; GPIO16/17 are used by Waveshare resistive touch.

Enable `FEATURE_WM8960_USB_AUDIO` in
`main/keyer_features_and_options_pico.h`. The project compiles TinyUSB's audio
class driver as an application class driver only when this option is enabled,
so no Arduino-Pico core rebuild or TinyUSB force-include flags are required.
It also compiles a two-port CDC driver in place of the core's single-port
driver and the MIDI driver in place of the core's weak MIDI stubs.
The asynchronous audio endpoints reserve 49 sample frames per USB millisecond
(196 bytes stereo playback, 98 bytes mono capture), allowing clock variation
around 48 kHz and Windows UAC2 driver startup.
The composite USB layout includes the built-in CDC
console, a second CDC interface, MIDI, USB speaker playback, and USB microphone
capture. WinKey uses the first CDC, the terminal uses the second, and the MIDI
application routing is described below. USB disconnects and reconnects once during startup to
install the composite descriptors, so a serial monitor may need to reconnect.
The codec must be connected and initialize successfully for audio to work.
Audio is serviced during keyer timing waits as well as the main loop, so CW
elements continue generating PCM samples while keying. Opening the primary
serial port reports whether codec initialization succeeded. USB playback FIFO
access uses Arduino-Pico's USB mutex to avoid interrupt-time stream-reset
deadlocks.
The serial status includes DMA progress in units of 128 frames, polled from
ring positions rather than counted by interrupt callbacks. Progress can
undercount if servicing is delayed by a complete ring revolution.
For isolated crash testing, disable `FEATURE_MCP23017_EXPANDER`, `FEATURE_GPS`,
`FEATURE_COMPASS`, `FEATURE_BT_KEYBOARD`, `USE_CORE1`, and
`USE_CONNECT_ON_CORE1` in the active display profile. With both core-1 options
disabled, the Arduino shim does not launch a second application core.
The current isolated wiring uses BCLK18, LRCLK19, DOUT6, DIN7, SDA20/SCL21,
direct paddle GPIO2/3, straight-key GPIO4, TX14, and PTT22.
Bluetooth keyboard, core-1 GPS/background Bluetooth connection, and compass
were re-enabled for integration testing. GPS/compass are currently disabled
again to isolate the BT/audio crash; Bluetooth and core 1 remain enabled.
GPS RX remains GPIO5 at 38400 baud; the compass uses SDA20/SCL21 when enabled.
MCP23017 remains disabled. Direct paddles are now left GPIO3/right GPIO2,
with straight key GPIO4 and PTT22 unchanged. Paddle audio began
working during DAC-header probing and continued after probe removal; an
intermittent connection is suspected, not a confirmed firmware root cause.
Routine audio-stage and post-audio startup messages are suppressed on the LCD.
Initialization failures and retained crash diagnostics remain visible.
The sketch-local RP2 USB device driver also carries the SDR project's TinyUSB
ISO-reactivation backport: an active transfer is aborted before Windows reopens
an audio streaming interface. Without this build-overlay fix, the core can panic
with `ep ... was already available` during format selection or stream restart.
Temporary timeout diagnostics display `WD S:... R:... T:...` separately from
HardFault `LR`/panic diagnostics. R/T are DMA completion counts modulo 64.
Stage 1 is initialization, 2 is initialization complete, 3 is serial status,
4 is control initialization, 5 is USB playback, 6 is I2S capture, 7 is I2S
rendering, and 8 means audio service returned to the keyer. The second row
retains the last USB trace and interface/alternate-setting request; those are
not fault addresses.
Additional keyer checkpoints are 9 (Bluetooth initialization), 10 (serial/keying),
11 (touch), 12 (Bluetooth status/events), 13 (keyboard processing),
14 (configuration/memory maintenance), and 15 (display and remaining services).
Calls to audio service within these operations can replace their checkpoints.
Reopening the primary CDC serial port with DTR asserted also reports accepted
tone starts, the last tone frequency, tone frames submitted to I2S, and the
four I2S GPIO functions. This distinguishes missing tone generation from
pin reassignment or codec/output-path silence.
Serial status also samples LRCLK transitions over 2 ms and reports cumulative
USB playback bytes and peak playback/ADC sample magnitudes since boot.
The brief polling test checks GPIO clock activity, not precise clock frequency
or the signal reaching the codec.

### USB MIDI

The Pico WM8960 composite interface now carries the MIDI behavior from
`D:\Documents\GitHub\RP2350-SDR-CW-Interface`. The previous keyer MIDI code
provided only the USB descriptor and driver, not application handlers.
ESP32 and non-WM8960 builds do not gain a MIDI interface.

Outgoing events use MIDI channel 10 and cable 0:

| Event | Message |
|---|---|
| CW key down/up from paddles, straight key, WinKey, keyboard, or memories | Note On 17, velocity 127 / 0 |
| PTT in `ONOFF` mode | Note On 18, velocity 127 / 0 |
| PTT in `TOGGLE` mode | Note On 18, velocity 127 at both edges |
| Positive / negative wheel step | CC20, value 127 / 1 |
| Keyer speed | CC21, Thetis CW Speed scaling for 1..60 WPM |
| Optional sidetone pitch | CC3, 300..1000 Hz mapped to 0..127; higher pitches clamp to 127 |

Command-mode practice and disabled transmitter keying do not send CW notes.
MIDI PTT follows the existing logical PTT lead/tail and manual PTT controls;
it does not replace the GPIO PTT output. The saved `midi_ptt_mode` field
defaults to `OFF` for host-controlled Semi Break-In. Thetis CW and PTT have
been user-tested successfully with `ONOFF`. Use `ONOFF` for piHPSDR.
SDRoxide PTT has been user-tested successfully with `TOGGLE`; CW remains unresolved.
Do not use `TOGGLE` with Thetis.

Terminal commands (CDC2 when configured) are:

- `\:MIDI`: show connection, PTT mode, queue, and packet/error counters.
- `\:MIDI-PTT [OFF|ONOFF|TOGGLE]`: show or change the saved PTT mode.
  Release PTT and let queued events drain before changing it.
- `\:MIDIWHEEL +` / `\:MIDIWHEEL -`: send one wheel step for host mapping/testing.

`wm8960_midi_wheel_step(int direction)` supplies the wheel API; positive
direction sends 127 and negative sends 1, matching the source implementation
(its README lists the opposite values). The Pico WM8960 encoder menu below
uses this API for VFO tuning.

Uncomment `SEND_MIDI_FREQUENCY` in `main/keyer_features_and_options_pico.h`
to send CC3 when its mapped value changes and once per second. It defaults
off to avoid disrupting host MIDI assignment dialogs.
`DEBUG_MIDI` in `main/keyer_debug.h` enables routine MIDI status on terminal
connection. Errors remain reported without that define; explicit status/help
commands remain available.
`DEBUG_MIDI_MESSAGES` enables individual RX/TX packet traces on the same
terminal. RX includes packets later rejected or suppressed as echoes; TX
means accepted by TinyUSB, not proof of host receipt. Each line contains
the observation timestamp in microseconds, all four USB-MIDI bytes in hex,
cable/CIN, one-based channel (0 for system messages), decoded type, and
decimal data bytes. For CC messages, `data1` is the controller number and
`data2` is its value.
Traces buffer up to 128 messages and drain one line per service call only
when key/PTT sources are idle and terminal space is available. Overflow is
reported as a dropped-message count. Open the CDC2 terminal with DTR enabled,
change Thetis CW Speed to two or more known WPM values, and record the
corresponding RX lines to establish its mapping. Thetis speed-to-keyer/LCD
control is not implemented; speed synchronization instead runs from the keyer
to the host using CC21 on channel 10. Assign this controller to CW Speed in
Thetis. The value uses the inverse of
[Thetis's `CWSpeed` conversion](https://github.com/ramdor/Thetis/blob/master/Project%20Files/Source/Console/Midi2CatCommands.cs):
`round((WPM - 1) * 2.1333)`, with reported WPM clamped to 1..60.
Value 1 is sent for 1 WPM because Thetis ignores value 0.
For example, 5 WPM sends 9, 20 WPM sends 41, and 50 WPM sends 105.
Raw WPM values previously displayed about half-speed in Thetis because it
decodes `round(value / 2.1333 + 1)`; neighboring values can decode to the
same WPM. Actual keyer speed is not clamped or changed. The keyer
sends it on USB connection/reconnection, whenever `configuration.wpm` changes,
and every five minutes while connected. This includes changes from local
controls, WinKey, and buffered speed commands; command-mode practice speed
and Farnsworth spacing settings are separate and are not reported.
Existing local speed/LCD behavior is unchanged. No EEPROM layout change is
needed. This CC21 encoding targets Thetis's absolute CW Speed action;
other hosts need an equivalent mapping.

Incoming Note On 17 (nonzero velocity) starts a WM8960 monitor tone using
the current sidetone frequency/volume. Zero velocity or Note Off releases it.
Note 18 controls USB priority mute only. These notes are accepted on any MIDI
channel, tracked independently per channel, and **never drive key/PTT GPIO**.
CC120/CC123 release the corresponding channel's incoming key/PTT state.
Incoming key/PTT states mute both USB audio directions with the existing
5 ms fades; they release on note-up without adding a seven-dit hold.
The local keyer's seven-dit hold remains unchanged. Local sidetone has
priority if both local and MIDI tones are active; releasing either source
does not cancel the other.

For a manual MIDI monitor test, send `99 11 7F` and then `99 11 00`
(three-byte MIDI messages, not four-byte USB packets).
Received cable-0 packets are echoed; matching recently transmitted packets
are suppressed for 15 ms to prevent immediate host loopback. Avoid routing
the device output back into its input; delayed host loops cannot be
distinguished reliably from new commands.

USB access uses Arduino-Pico's USB mutex, with bounded foreground servicing
during keyer waits. A 128-packet queue retries temporary endpoint backpressure.
Overflow or a queued packet reaching 250 ms stops MIDI output, discards stale
events, and retries a CW/PTT release; reconnect USB to resume output.
Local GPIO/audio keying remains operational. USB disconnect/suspend releases
incoming MIDI audio states and discards queued events. Reconnect reports the
current CW state but does not replay a PTT toggle or stale PTT hold.
User-reported host interoperability:

| Host | MIDI PTT mode | CW note 17 | PTT note 18 |
|---|---|---|---|
| Thetis | `ONOFF` | Working | Working |
| SDRoxide controlling an Elecraft K3 | `TOGGLE` | Not yet working; investigation ongoing | Working through to radio TX |
| Quisk | Not yet established | Not yet working | Not yet verified |

Thetis CW Speed synchronization from CC21 is also user-confirmed working
after applying the Thetis-compatible scaling above.

Incoming MIDI and other host-radio interoperability tests remain pending.
Host regression tests compile the actual MIDI handlers and audio source/render
functions with mocked USB/DMA backends. On Windows with Visual Studio C++
tools installed, run `.\tests\wm8960_midi_host_test.ps1`; it checks debug/pitch
options both enabled and disabled, packet routing, source coexistence,
EEPROM configuration byte round-trips, and USB failure recovery.

### Pico MCP23017 TX/PTT outputs

With `FEATURE_MCP23017_EXPANDER`, all six TX/PTT pairs use the expander:

| Transmitter | TX key MCP pin | PTT MCP pin |
|---|---|---|
| 1 | 3 (A3) | 4 (A4) |
| 2 | 5 (A5) | 6 (A6) |
| 3 | 7 (A7) | 8 (B0) |
| 4 | 9 (B1) | 10 (B2) |
| 5 | 11 (B3) | 12 (B4) |
| 6 | 13 (B5) | 14 (B6) |

Paddles remain on MCP pins 0/1, straight key on pin 2, and pin 15 is spare.
The TX Select touch control cycles through six transmitters. Internal output
identifiers use 100 + MCP pin; paddle/key identifiers remain raw MCP pins.
CPU GPIO14/22 are no longer TX/PTT outputs in this mode; GPIO22 remains the
expander interrupt input for the current Pico display profile.
Without the feature, existing GPIO assignments and TX selection limits remain
unchanged. No EEPROM format/magic change is required: the saved transmitter
selection is resolved to the current pin mapping at startup.

Output latches are set to the configured inactive polarity before enabling
each output direction. Writes use cached OLAT values to preserve other outputs
without clearing paddle interrupts, and report I2C write errors. On Pico,
interrupts only flag input changes; foreground paddle/straight-key reads
service the expander, avoiding I2C reentry during TX/PTT writes.
Only key input pins generate interrupts. Expander output updates add I2C
latency; hardware timing and integration with the codec still need testing.
Run `.\tests\keyer_expander_host_test.ps1` for the host routing/polarity tests.

### Pico WM8960 rotary encoder

`FEATURE_ROTARY_ENCODER` is enabled in the Pico configuration. The user's
pin assignments in `main/keyer_pin_settings_pico.h` are **CPU GPIO numbers**,
not MCP pin numbers:

| MCP23017 feature | Encoder A/B | Switch |
|---|---|---|
| Enabled | GPIO2 / GPIO3 | GPIO4 |
| Disabled | GPIO22 / GPIO14 | GPIO42 |

These conditional assignments reuse CPU pins freed by moving key inputs
and TX/PTT outputs onto the MCP23017. The current direct-GPIO profile disables
TX/PTT GPIO outputs (zero) to free GPIO14/22. Internal pull-ups are enabled;
connect encoder contacts/switch to ground. Review assignments before changing
features or restoring direct TX/PTT pins.

With a nonzero `rotary_switch`, the initial function is MIDI VFO tuning.
Short press/release advances through:

**VFO -> master (USB playback) volume -> sidetone volume -> sidetone
frequency -> CW speed -> USB microphone level -> VFO.**

The switch has 20 ms debounce. Holding it for one second returns to VFO;
release after a long hold does not advance the menu. There is no double-click
save action. Non-VFO function/value messages appear on the LCD for two seconds;
rotation updates the controls immediately, with one final-value message after
100 ms without further processed steps rather than a redraw per step.
VFO uses only the status letter, without a per-step LCD message.
The status row immediately to the right of WPM shows a light-green mode letter:
**V** = VFO, **M** = master volume, **S** = sidetone volume,
**F** = sidetone frequency, **W** = CW speed, **U** = USB microphone level.
Output routing (headphone/speaker/both/none) and decoder-source selection
are not included.

Setting `rotary_switch` to **0** disables all switch pin setup/reads and fixes
the encoder function to **master/USB playback volume**, not speed.
Menu selection is not saved; audio/pitch/speed settings use normal deferred
EEPROM saving, with no layout or magic-number change.

Volume functions reuse the existing approximately 1 dB audio-taper steps and
unmute the selected path. Master volume changes the existing saved USB playback
trim, not the codec's analogue output gain or sidetone volume. Frequency
changes by 10 Hz per decoded step, limited to 300..1200 Hz. CW speed retains
the existing 1/2-WPM acceleration, command-mode speed handling, WinKey speed
notification, LCD update, and normal-speed MIDI CC21 synchronization.
VFO emits one CC20 relative message per decoded step (positive 127, negative 1).
`ENCODER_VFO_DIRECTION` in `main/keyer_settings_pico.h` defaults to **-1**
to reverse VFO tuning; set **+1** for the original direction. Other modes
are unaffected.
The existing half-step quadrature decoder is reused. Pico GPIO CHANGE
interrupts capture transitions and queue up to 127 timestamped steps while
foreground display/audio work runs. Only foreground code changes settings,
draws the display, or sends MIDI; queue overflow reports an explicit error
on the terminal. All modes drain up to 16 captured steps per encoder service call
and immediately services MIDI, without a periodic send timer. Other builds retain
the legacy speed-only encoder behavior.

`potentiometer` set to **0** disables `FEATURE_POTENTIOMETER` after pin/settings
selection, even if the feature was requested. No analog-speed pin or enable-pin
setup, ADC reads, or potentiometer enable/help/UI commands are compiled in.
Saved activation is ignored/cleared when settings load. A nonzero
potentiometer pin retains existing feature-controlled behavior.

Host menu/button/volume tests and Pico builds with MCP23017 enabled/disabled
pass; physical encoder/menu testing remains pending.

### Pending Pico integration worklist

All items below are pending; order follows the requested worklist.

1. Test with GPS, MCP23017, and compass features enabled; resolve pin
   assignments and conflicts.
2. Diagnose touch on the currently configured Pico display: normal touch
   operation requires first pressing just the right spot at the far-left
   center edge of the panel. Determine whether the cause is software or
   hardware.
3. Test Command mode using a single button.
4. Add touch buttons for volume and mute controls, usable without a serial
   terminal or keyboard.
5. Port this project's Pico Bluetooth keyboard support into
   `D:\Documents\GitHub\RP2350-SDR-CW-Interface`.

- Mike K7MDL

********************************
*****************************************
