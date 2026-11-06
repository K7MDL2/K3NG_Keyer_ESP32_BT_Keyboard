| Support Targets | ![ESP32-WROOM32][esp32] | ![Pico2W][Pico2W] | ![alt text][Has_Precompiled_Firmware_Images] | ![M5Stack_Core2][Core2] |
| --- | --- | --- | --- | --- |

| Dev Environment Used | ![alt text][ESP-IDF6] | ![Arduino IDE][Arduino]|
| --- | --- | --- |

| BT Keyboards Tested | ![alt text][K380]|![alt text][K380s] | ![Rii i8+ mini][Rii_mini] |
| --- | --- | --- | --- |

[Pico2W]: https://img.shields.io/badge/Pico2W-green "ESP32"
[Core2]: https://img.shields.io/badge/M5Stack-Core2-blue "Core2"
[ESP32]: https://img.shields.io/badge/ESP32--WROOM-orange "ESP32-WROOM"
[ESP-IDF6]: https://img.shields.io/badge/ESP--IDF--v6.1.0-cyan "ESP-IDF v6.0.1"
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

With arduino-cli the bundled `components` folder can be used as the library source directly, avoiding the copy of libraries into the Arduino libraries folder (and any accidental use of unmodified upstream versions). For example, a Pico build for the Waveshare RP2350B Plus W:

```
arduino-cli core update-index --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli core install rp2040:rp2040 --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli compile --fqbn rp2040:rp2040:waveshare_rp2350b_plus_w:ipbtstack=ipv4btcble --libraries components -e main
```

`--libraries components` takes priority over copies in the Arduino libraries folder, so the repo's modified TFT_eSPI, MCP23017, TimeLib, and TinyGPSPlus are always the versions built. The UF2 is written to `main/build/<fqbn>/main.ino.uf2`. The bundled TFT_eSPI fork's `User_Setups` folder contains wrapper headers that include the display setup files from `TFT_e_SPI_Custom_Config`, so no TFT_e_SPI_Custom_Config copy step is needed when building from `components` (for Pico or ESP32 displays).

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
 - Sept 2026 have BT Classic and BLE keyboards pairing with receonnection on both ESP32 and Pico2W. The BT Classic keyboard, Logitech K380, pairs with a PIN the first time, then after, pairs to receonnct, no PIN required so it is reasonably fast.
 ## Platform matrix and cleanup baseline

 - This fork retains ESP32, Pico (RP2040/RP2350), and Teensy 4.0/4.1 as its CPU scope. Architecture selection alone does not mean that every board has a working firmware profile.

| Target | Build path and configuration | Status |
| --- | --- | --- |
| ESP32-WROOM32 / M5Stack Core2 | ESP-IDF; `keyer_features_and_options_esp32_dev.h`, `keyer_pin_settings_esp32_dev.h`, `keyer_settings_esp32_dev.h` | Existing profiles retained, including TFT and text-LCD/no-display options. ESP-IDF is required for full supported Bluetooth keyboard operation. |
| ESP32 under Arduino | Same ESP32 headers | Compile compatibility retained; not equivalent Bluetooth keyboard coverage to ESP-IDF. |
| Pico / Pico W / Pico 2 / Pico 2 W (RP2040/RP2350) | Arduino-Pico; corresponding `keyer_*_pico.h` headers | Existing architecture selection retained. This cleanup's full firmware build was checked on Waveshare RP2350B Plus W; other boards still require their pin/display/audio configuration to be verified. |
| Waveshare RP2350B Plus W with WM8960 | Arduino-Pico, `waveshare_rp2350b_plus_w`, Bluetooth stack enabled, 4 MB flash with 64 KB filesystem | Current primary Pico configuration: dual CDC serial, USB audio input/output, MIDI, codec tones and local CW decoding retained. |
| Teensy 4.0 / 4.1 | Teensyduino, IMXRT1062 | Architecture retained; firmware bring-up pending. No feature/pin/settings profile exists yet, so a build reports an explicit pending-profile error instead of selecting missing generic headers. |
| AVR boards, Due/SAM, SAMD, STM32, PIC32, Teensy 2.x/3.x | None in this fork | Unsupported CPU selection is rejected; obsolete board-profile include branches have been removed. |

- The first cleanup milestone changes board selection only and extracts the pure foreground-stage diagnostic name lookup into [keyer_diagnostics.cpp](main/keyer_diagnostics.cpp) and [keyer_diagnostics.h](main/keyer_diagnostics.h). Arduino compiles the new translation unit alongside the sketch; ESP-IDF lists it explicitly in [main/CMakeLists.txt](main/CMakeLists.txt). [main.ino](main/main.ino) is already a sketch shim; the implementation being gradually split is [main.cpp](main/main.cpp).

- No pin assignments, EEPROM layouts, feature defaults, decoder algorithms, sidetone methods, or scheduling are changed by this milestone. Disabled optional features are not automatically dead code: valid text-LCD, digital/analog decoder, and GPIO sidetone paths remain for later platform-by-platform review.

November 3, 2026 added:
See https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Operator-Guide

- Added M5Stack Core2 with audio and limited inut an output pins.
- Pico only, USB sound card, MIDI functionality, encoder+switch, are now working and compiles with Arduino IDE as before.
- Uses common Waveshare WM8960 module with headphone, speakers and an onboard mic.  Can use as a soundcard for normal audio output. Sidetone is generated locally for zero latency and played into the USB audio output.  USB audio (play and mic) are muted for 7 dit lengths enabling full break-in if your hardware allows it.  Serial terminal commands, BT keyboard volume +/-/Mute keys work and are saved in EEPROM.  Adding touch controls later.  The USB MIDI/Audio/Serial configuration was developed as an overlay over standard libraries in this project https://github.com/pa3gsb/RP2350-SDR-CW-Interface using the Pico SDK.  Thank you to PA3GSB for working that out.  It was applied in this project with changes to make it work under Arduino IDE. The existing MIDI, serial lines, encoder were then wired up to the new USB interfaces.
- MCP23017, GPS working.  Compass not wired to test yet, should work.
- MIDI now sends several messages and can read them if the app supports that.  Tested with Thetis, sdroxide, piHPSDR and deskHPSDR so far.  Set up CC21 in Thetis for CW Speed knob and it will report the keyer speed setting and stay sync'd to changes and current speed on the keyer side.  Same for sidetone frequency for apps that support it.  There are 3 MIDI operation modes.  See Wiki.

   All locally generated messages use **MIDI channel 10**.

   | Function | Message | Values |
   |---|---|---|
   | CW key | Note 17 | Down: 127; up: 0 |
   | PTT | Note 18 | `OFF`: none; `ONOFF`: 127/0; `TOGGLE`: 127 on both edges |
   | VFO encoder | CC20 | Positive direction: 127; negative: 1, after configured reversal |
   | CW speed | CC21 | Thetis scaling: `round((WPM − 1) × 2.1333)`; WPM clamped to 1–60; minimum output value 1 |
   | Master/USB playback volume | CC7 | 0–100% mapped to 0–127, rounded |
   | Sidetone volume | CC22 | 0–100% mapped to 0–127, rounded |
   | Sidetone frequency | CC3 | 300–1000 Hz mapped to 0–127, clamped and rounded |
   | USB microphone level | CC23 | 0–100% mapped to 0–127, rounded |

   - Key and PTT releases use Note On messages with velocity 0, except in PTT `TOGGLE` mode.
   - Volume mute sends **0**; unmute reports the stored level.
   - Audio controls report changes and USB connection/reconnection, without periodic repeats.
   - CW speed reports changes, USB connection/reconnection, and every **five minutes**.
   - Reconnection reports the current CW state but does not replay PTT.
   - Accepted incoming cable-0 messages are echoed; matching recently transmitted messages are suppressed for 15 ms to prevent immediate loopback.
   - Optimized to prevent key timing and tone distortions.  GPS and most screen updates are paused dering keying and PTT.

- There are now 2 serial ports, one for Winkeyer and the 2nd keyer's terminal interface and debug info.
- Up to four CPU encoders can control assignable audio, VFO and RX decoder functions. First press opens a compact Select/Adjust touch menu; long press closes it. All functions initially belong to encoder 1. Without a configured push switch, encoder 1 initially controls USB playback volume.
- When the MCP23017 is enabled, All 6 PTT_TX and 6 TX_Key_line outputs are mapped to MCP23017 expander pins freeing up local GPIO for encoders, serial GPS, and I2S lines for the audio module.  The Paddle and Straight key inputs were already mapped there.
- CW text box scrolling and clear screen perf improvement.
- Several audio related optimixations for celar tones and voice from mic and PC.
- Several new serial termina configuration commands for Audio, Mic, and NIDI.
- Tested MIDI and audio with sdroxide, deskHPSDR, puHPSDR, Thetis on Windows and Linux Mint.

Check out the latest main branch changes at the Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Change-Notes

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

The BT keyboard code translates HID reports into the shared `BT_KEY_*` command codes and calls `service_keyboard_commands`, also used by touch buttons. Physical PS/2 and USB-host keyboards are no longer supported. The existing command assignments are preserved on this Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/BT-Keyboard-Key-Assignments. Not all keys from a full-size keyboard are present on compact BT keyboards.

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
configured `eeprom_write_time_ms` before removing power. On Pico WM8960 builds
a save also waits until settings have been unchanged for 2 s (so a slider or
encoder sweep is one flash write), and the speaker output is faded out before
the ~47 ms flash write, which stops both cores, so the stall is a brief
silence instead of a pop.

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
| `\:LOCALRX 25` | Set local WM8960 USB-playback listening level to 25%, even in MIDI POD mode |
| `\:USBMIC 80` | Set USB microphone level to 80% |
| `\:SIDETONE 50` | Set sidetone to 50% |
| `\:USBPLAY MUTE` / `\:USBPLAY UNMUTE` / `\:USBPLAY TOGGLE` | Change playback mute without losing its level |
| `\:USBMIC +` / `\:SIDETONE -` | Adjust the selected level by 2% (linear) |

All four target commands accept a level from 0 to 100, `+`, `-`, `MUTE`,
`UNMUTE`, or `TOGGLE`; no argument shows status. Setting/adjusting a level
unmutes that target. Zero level is silent. Invalid input reports an error
without changing settings. `\S` also includes audio status; `\?` lists commands.

Bluetooth consumer/media keys use the same controls:

| Prefix | Mute / Volume+ / Volume- target |
|---|---|
| None | USB playback |
| Left or right Shift | WM8960 sidetone |
| Left or right Ctrl | USB microphone |

Mute toggles the selected path; Volume+/Volume- use linear 2% steps and
unmute, as do terminal `+`/`-` commands (the earlier approximately 1 dB
audio-taper steps were too coarse near full scale). 0% is silence and 100%
is the upper limit. Numeric commands still set
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

- `\:MIDI`: show connection, PTT mode, control mode, queue, and packet/error counters.
- `\:MIDI-PTT [OFF|ONOFF|TOGGLE]`: show or change the saved PTT mode.
  Release PTT and let queued events drain before changing it.
- `\:MIDI-MODE [LOCAL|REPORT|POD]`: show or change the saved control mode (below).
- `\:MIDI-VFO [NORM|REV]`: show or change the saved VFO tune direction.
  `REV` reverses the encoder's VFO wheel steps relative to the compiled
  `ENCODER_VFO_DIRECTION` default.
- `\:MIDIWHEEL +` / `\:MIDIWHEEL -`: send one wheel step for host mapping/testing.

MIDI control modes decide what the keyer's own controls (encoder, BT keyboard
media keys, terminal commands) do. Keys, PTT, and the VFO wheel work in every
mode, and the WM8960 sidetone always plays locally.

| Mode | Master volume, USB mic level | Speed, sidetone freq/level | Reported to PC (CC) |
|------|------------------------------|----------------------------|---------------------|
| `LOCAL` | local | local | nothing |
| `REPORT` | local | local | all |
| `POD` (default) | **PC only, mirrors PC** | local, **PC can set** | all |

`POD` makes the keyer a control pod: set the local levels in `LOCAL` or
`REPORT`, then switch to `POD`. Master volume (CC 7) and USB mic (CC 23)
changes then go only to the PC, starting from the local levels, while the
keyer's own WM8960 playback and mic gains stay fixed. When the PC sends CC 7 or
CC 23 on channel 10, the keyer mirrors that value (so the next knob step
continues from the PC level) without changing local gain or sending it back.
The PC can also set CW speed (CC 21, Thetis scaling; value 0 ignored),
sidetone level (CC 22) and sidetone frequency (CC 3, 0..127 = 300..1000 Hz)
on channel 10. These update the keyer and its WM8960 sidetone, are saved to
EEPROM, and are not sent back. `LOCAL` and `REPORT` ignore all PC control values.
The PC-side levels are not saved; they restart from the local levels after a
reboot or mode change. `\:AUDIO` also shows them in `POD` mode. USB MIDI has no
"port open" signal, so the mode is a manual setting. Set the build default with
`WM8960_MIDI_CONTROL_MODE` (0=LOCAL, 1=REPORT, 2=POD). Adding this saved setting
changed the EEPROM layout, so the first boot of this build resets saved settings.

`wm8960_midi_wheel_step(int direction)` supplies the wheel API; positive
direction sends 127 and negative sends 1, matching the source implementation
(its README lists the opposite values). The Pico WM8960 encoder menu below
uses this API for VFO tuning.

These controls send absolute MIDI CC values on channel 10 when changed
and on USB connection/reconnection, without periodic repeats:

| Control | CC | Mapping to 0..127 |
|---|---|---|
| Master (USB playback) volume | 7 | 0..100%, rounded |
| Sidetone volume | 22 | 0..100%, rounded |
| Sidetone frequency | 3 | 300..1000 Hz, clamped and rounded |
| USB microphone level | 23 | 0..100%, rounded |

Muted volume controls report 0; unmuting reports the stored level.
Changes from the encoder, keyboard, terminal, and restored settings are
covered. Frequency reporting no longer requires `SEND_MIDI_FREQUENCY`.
For host MIDI assignment dialogs, avoid changing other controls while mapping.
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

### USB microphone PTT gate and host feedback

`WM8960_MIC_GATE_MODE` in `main/keyer_features_and_options_pico.h`
selects microphone gating at build time:

| Value | Microphone permitted when |
|---|---|
| 0 | Always (legacy mode, including legacy incoming MIDI PTT muting) |
| 1 | Manual local PTT is active |
| 2 | Selected external MIDI PTT feedback is active |
| 3 (default) | Either manual local PTT or selected external feedback is active |

Manual local PTT includes Ctrl-U, touch/terminal PTT commands, and the PTT
input, not automatic CW/tune PTT. Tone/key monitor muting and its release
hold still take precedence, as do the existing microphone mute/level controls.
The host USB capture stream remains connected and receives silence while gated.
This changes the default from always-open capture; choose mode 0 for an
external transmitter whose PTT cannot be observed by the keyer.
The saved `\:MIC GATE 0..3` terminal command changes the mode at runtime;
`WM8960_MIC_GATE_MODE` is only the default after an EEPROM reset.

SDRoxide PTT LED feedback defaults to channel 10, Note 18:
`WM8960_PTT_FEEDBACK_CHANNEL=10`, `WM8960_PTT_FEEDBACK_TYPE=9`,
`WM8960_PTT_FEEDBACK_NUMBER=18`. Captured feedback uses velocity 127 for
PTT on and zero for off. Select **Keyer MIDI** as SDRoxide's **Feedback to**
destination and enable the PTT LED option. Set the feedback channel to 0
to disable matching. `DEBUG_MIDI_MESSAGES` remains enabled for testing.
Toggle SDRoxide's PTT on and off and record the `[MIDI RX]` lines on CDC2
(DTR enabled). Traces include channel, type, controller/note, and value, even
for packets subsequently suppressed as immediate echoes. Trace printing is
deferred while key/PTT sources are active.

For other mappings, set `WM8960_PTT_FEEDBACK_CHANNEL` to the message's
one-based channel (1..16), `WM8960_PTT_FEEDBACK_TYPE` to 11 for CC or 9 for
Note On/Off, and `WM8960_PTT_FEEDBACK_NUMBER` to its controller/note.
Nonzero means active; zero or Note Off means inactive. Matched feedback is
consumed rather than echoed, and is matched before echo suppression because
host state reports can be byte-identical to the keyer's own PTT message.
Hosts that loop the keyer's own messages back (MIDI thru) are not supported
as feedback sources. Disconnect, suspend, and reconnect clear its state.
Other host value encodings need an explicit decoder once captured.
Default mode 3 opens the mic for manual local PTT or this external feedback.

Host mic-level feedback is **off by default** (`WM8960_MIC_LEVEL_FEEDBACK_CHANNEL
0`): SDR apps such as SDRoxide treat the incoming mic level as fixed and adjust
their own gain, so syncing both ends fights. Volume and sidetone levels are
likewise not synced from the host; only keyer-owned settings (CW speed and
sidetone frequency) are candidates for host sync. If enabled (e.g. channel 10,
CC 23, the same message the keyer sends for its USB mic level), the keyer
sets its USB mic level from the value (0..127 scaled to 0..100%), unmutes the
mic, and saves the level to EEPROM after the normal settle delay (never while
PTT is active). The message is consumed rather than sent back, and feedback of
a level the keyer itself reported within the last second is ignored.
Change `WM8960_MIC_LEVEL_FEEDBACK_CC` for a different controller.

The WM8960's analog microphone gain is set at build time by
`WM8960_MIC_BOOST` (0 = 0 dB, 1 = +13 dB, 2 = +20 dB default, 3 = +29 dB)
and `WM8960_MIC_PGA` (0..63, -17.25 dB + 0.75 dB per step; default 33 =
+7.5 dB). These apply before the ADC, so lower them if the `[MIC]` debug input
peak reaches 32768 (clipping); the keyer and host mic levels cannot remove
that distortion. Use the keyer mic level to set the transmitted loudness.

Set `WM8960_MIC_ALC 1` to let the codec's automatic level control adjust the
PGA instead (default off). `WM8960_MIC_ALC_TARGET` sets the target level
(0..15, -22.5 dBFS + 1.5 dB per step; default 7 = -12 dBFS) and
`WM8960_MIC_ALC_MAX_GAIN` caps the PGA gain (0..7, -12 dB + 6 dB per step;
default 4 = +12 dB). A noise gate at -61.5 dBFS holds the gain between words so
background noise is not pumped up; ALC can still raise room noise compared
with fixed gain.

To tune without rebuilding, use the `\:MIC` CLI command. `MIC` alone prints
the current gain and a meter (peak, RMS and clipped-sample count of the ADC
input since the previous `MIC`). `MIC PGA n`, `MIC BOOST n` and `MIC ALC ON|OFF`
change the codec gain immediately and are saved to EEPROM after the normal
settle delay; the build defines are only the defaults after an EEPROM reset.
`MIC GATE 0..3` changes and saves the microphone PTT gate mode (above).
Adding these saved settings changed the EEPROM layout (Pico magic 45), so the
first boot of this build resets saved settings. `MIC TONE` is not saved.
`MIC TONE ON` replaces the ADC input with a clean 1 kHz tone
at -6 dBFS. If the tone is clean on air, the distortion is in the
microphone/analog stage rather than in the USB, PC or transmit chain.
`MIC TONE OFF` restores the microphone.

The codec and MCP23017 share one I2C bus next to the I2S clock pins, so long
or poorly grounded wiring lets the 3 MHz BCLK corrupt I2C transfers.
`\:I2CTEST [n]` (default 100, max 1000) does n codec register writes and n
MCP23017 register reads with the I2S clocks running, then repeats them with
BCLK/LRCLK held low, and prints the success and error counts for each.
Audio drops out briefly during the second half. Failures only while the clocks
run point to wiring: keep I2C and I2S wires short, add a ground wire beside
them and use 2.2k-4.7k I2C pull-ups. The firmware drives the I2S outputs with
slow slew and 2 mA to reduce this coupling, retries failed codec writes, and
retries failed paddle reads (keeping the MCP23017 interrupt pending until a
read succeeds).

Host PTT feedback also drives the keyer's PTT output, keeping the keyer and
host in step:

- Host PTT on while the keyer is idle keys the keyer PTT (GPIO/MCP, T/R
  status icon) as a manual PTT, without sending MIDI back.
- Host PTT off releases a manual keyer PTT (Ctrl-U, touch/terminal, or
  host-started) without sending MIDI, so a toggle host is not switched back
  on. A held PTT input pin is left alone; if a key is down, the normal PTT
  tail releases it.
- Automatic CW/tune PTT is never converted to a held manual PTT.
- Releasing host-started PTT locally (Ctrl-U) still sends the off message.

For a host whose PTT control toggles on each press (SDRoxide **Toggle**),
use `\:MIDI-PTT TOGGLE` so each Ctrl-U press sends one Note On 127. With
SDRoxide **Hold**, use `\:MIDI-PTT ONOFF`.

Incoming Note On 17 (nonzero velocity) starts a WM8960 monitor tone using
the current sidetone frequency/volume. Zero velocity or Note Off releases it.
Note 18 controls USB playback priority mute and legacy-mode microphone mute
unless selected as external PTT feedback above. These notes are accepted on any MIDI
channel, tracked independently per channel, and **never drive key/PTT GPIO**.
CC120/CC123 release the corresponding channel's incoming key/PTT state.
Incoming key states (and PTT in legacy microphone mode) mute USB audio with the existing
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
tools installed, run `.\tests\wm8960_midi_host_test.ps1`; it checks debug
options both enabled and disabled, control-change reporting, packet routing, source coexistence,
EEPROM configuration byte round-trips, and USB failure recovery.

### M5Stack Core2 initial bring-up (ESP-IDF)

Select the Core2 profile without changing the default Hosyond profile:

```powershell
idf.py -B build-core2 -DKEYER_DISPLAY_TYPE=M5STACK_CORE2_LCD build
```

Alternatively select `DISPLAY_TYPE M5STACK_CORE2_LCD` in
`main/keyer_features_and_options_esp32_dev.h`. M5Unified/M5GFX provide Core2
power management, the built-in LCD and FT6336 capacitive touch. Other displays
continue to use TFT_eSPI. Bluetooth Classic and BLE retain the existing
ESP-IDF keyboard implementation and `USE_BLE` selection.

The three printed capacitive panel buttons are used: A cycles pages; B and C
activate the two actions shown above them. `BUTTON_ROWS` defaults to 4 for eight
actions, and may be set to 1-8 (eight pages exposes all sixteen actions).
`BUTTON_ASSIGNMENT_ORDER` in `main/keyer_settings_esp32_dev.h` sets their priority.
The default pages are Pause/TX enable, WPM up/down, Memory 1/2 and Memory 3/4.
The left label shows the current page/total. Short and very-long presses retain
the existing action behavior; touching the CW scroll box still aborts sending.
Core2 CW text uses three rows of twelve characters in FreeMonoBold18.
Startup text and timed messages use the smaller FreeMonoBold12 so prompts fit, centered in the
scroll box; touch-popup text is wrapped and centered line by line.

The provisional direct-GPIO map is:

| Function | Core2 GPIO | Connection |
|---|---:|---|
| Left paddle | 33 | Port A / M-Bus |
| Right paddle | 32 | Port A / M-Bus |
| Straight key | 14 | Existing buffered input; active-low, internal pull-up enabled |
| TX key 1 | 25 | ULN2803A; formerly BCD bit 0 |
| PTT 1 | 26 | ULN2803A; formerly BCD bit 1 |
| TX key 2 | 27 | ULN2803A; move former GPIO33 buffer input here |
| PTT 2 | 19 | ULN2803A; existing PTT output |
| Reserved input | 35 | Unused; input-only |

Paddles/key close to ground. Use appropriate radio-interface circuitry
(e.g. the existing ULN2803A Proto base), not radio voltages directly on GPIO.
The ULN2803A base retains GPIO25/26/19 outputs; move its former GPIO33 input
wire to GPIO27 to free Port A for paddles.
GPIO HIGH turns on each ULN2803A channel, pulling its collector output LOW;
GPIO LOW releases the output. GPIO14's reference PTT logic is active-low;
it is now the straight-key input, not manual PTT (`ptt_input_pin = 0`).
Both Port A paddle inputs have internal pull-ups, allowing the key to be
unplugged for keyboard-only operation. Port A is not available for external I2C.
Core2-only `\:PORTA` reports the active and saved connector mode.
Use `\:PORTA PADDLES` (default) or `\:PORTA OUTPUT`, then reboot to apply.
`\:PORTA REVERSE` toggles and saves polarity for both Port A outputs immediately,
including currently active key/PTT outputs. Connector mode changes still require
a reboot. Default OUTPUT polarity is GND on TX (key/PTT active), released on RX.
Reversed polarity grounds the outputs on RX and releases them on TX.
Both polarities use open-drain GPIO; a released output is not driven to 3.3V.
Port A output initialization and writes both use ESP-IDF GPIO APIs; Arduino
`digitalWrite` requires its own GPIO registration and cannot drive these pins.
`CORE2_PORT_A_REVERSED_DEFAULT` is 0 for GND on TX or 1 for GND on RX,
used when no polarity is saved. Paddle inputs and other TX outputs are unaffected.
In OUTPUT mode Port A GPIO33 is TX key 1 and GPIO32 is PTT 1; paddle
interrupts/reads are disabled and GPIO25/26 are left untouched. GPIO14 remains
the straight-key input and TX2 remains GPIO27/19. Select TX1 for Port A output.
`CORE2_PORT_A_OUTPUT_DEFAULT` in the ESP32 settings file is 0 for PADDLES or 1
for OUTPUT and applies when no saved mode exists. The mode is saved immediately
in separate NVS storage, preserving the existing EEPROM/memory layout.
Disconnect paddles before rebooting into OUTPUT mode. Port A provides raw
3.3V-rated open-drain GPIO, not protected radio connections; use suitable interface
circuitry and never connect radio voltages directly to it.
GPIO13 and input-only GPIO36
remain spare; the serial programming port, internal SPI/I2C, touch IRQ and
audio GPIO0/2/12/34 are not assigned to keyer inputs/outputs.

GPS, compass and expanders are disabled in this profile.
Ctrl-Q toggles the same saved beacon setting as serial `\_` when
`FEATURE_BEACON_SETTING` is enabled: repeated Memory 1 playback, also enabled
after reboot. A timed "Beacon On"/"Beacon Off" message confirms the setting.
Serial help lists `\_`.

`FEATURE_CORE2_AUDIO`
enables the built-in I2S speaker for sidetone and feedback beeps, without a GPIO
buzzer. `CORE2_SPEAKER_VOLUME` sets its maximum master level (0-255; default 96).
Core2 low/high feedback tones are 1000/1500Hz.
Existing sidetone enable/mode and frequency controls apply.
The extended terminal commands `\:AUDIO` and
`\:SIDETONE [0..100|+|-|MUTE|UNMUTE|TOGGLE]` now control Core2 speaker volume
and mute. Levels are percentages of `CORE2_SPEAKER_VOLUME` and are saved using
the normal delayed settings write. Plain consumer Mute/Volume Up/Volume Down
keys control the same speaker, with four-percentage-point steps; up/down unmutes.
Terminal `+`/`-` commands retain two-percentage-point steps.
Bluetooth volume/mute keys show the sidetone percentage or mute state for one
second using the same timed-message display as Pico encoder controls.
The Bluetooth task queues the latest level/mute snapshot; the main display path
renders it to avoid concurrent LCD font/alignment changes during rapid presses.
This affects sidetone and feedback beeps. USB playback/mic commands are rejected
on Core2 because those interfaces are not available.
Use `\:SIDETONE 42` for approximately the earlier reduced 40/255 master level.
Control characters such as a trailing LF from CR+LF terminal input are not
rendered as CW glyphs.
Disable
`FEATURE_CORE2_AUDIO` for silent display/GPIO-only operation. The microphone
remains disabled. Speaker DMA uses four 64-frame buffers at 48kHz for low latency;
test tone edges at your operating WPM before connecting a transmitter.
The Core2 ESP-IDF build overlays M5Unified's speaker task stack with a 4096-byte
base plus its audio buffer; the upstream 1536-byte base overflowed with this
ESP-IDF build. Managed library files are unchanged, and configuration fails
explicitly if an updated library no longer matches the overlay.
The Core2 build also overlays M5Unified's power code to use `std::isfinite`
with the ESP-IDF C++/picolibc toolchain. TFT_eSPI fonts remain available, but its
unused display-driver source is not compiled for Core2.
VS Code's ESP-IDF monitor requires the ELF in its configured build directory;
build that directory before starting the monitor. An ELF from a separate build
directory must match the firmware flashed to the device for reliable decoding.
The microSD, IMU and RTC are not initialized. Do not attach a conflicting
stacked module. Test without a connected transmitter first: screen orientation,
touch buttons/long presses, abort and text restart, paddles/straight key, then
both TX/PTT pairs. GPIO35 is reserved and is not sampled.

Run `.\tests\core2_gpio_host_test.ps1` to check GPIO exclusions, four/eight-page
action mapping, page cycling, short/long presses, abort and panel touch zones.

### Shared ESP32 display, GPS and key-input improvements

The ESP32 build uses the same incremental TFT scrolling, timed-message expiry,
GPS clock redraw, five-second GPS freshness check, sixty-second location
holdover and five-minute grid-change beep holdoff as the Pico build. Legacy
text-LCD redraw remains separate from the TFT path.
Touch, keyboard and terminal aborts reset the scroll text and its insertion
cursor, so new text starts at column 1, row 1 without a pending leading space.

ESP32 direct-GPIO paddles and straight key sample their initial levels after
interrupt setup, including a key held at startup. GPIO34/35 still require
external pull-ups. GPS UART reception uses a 1024-byte buffer configured before
the port starts to accommodate deferred GPS processing while transmitting.

These changes do not enable WM8960, USB audio or USB MIDI on ESP32.
Run `.\tests\esp32_shared_host_test.ps1` on Windows with Visual Studio C++
tools to test initial input states and edges, GPS freshness/holdover boundaries,
and incremental TFT character drawing and row repainting.

### Pico and ESP32 MCP23017 TX/PTT outputs

With `FEATURE_MCP23017_EXPANDER`, all six TX/PTT pairs use the expander:

Both platform pin-settings headers use output identifiers `100 + MCP pin`.
With the feature disabled, their existing direct-GPIO assignments are retained.
ESP32 expander enablement remains in its selected display profile in
`main/keyer_features_and_options_esp32_dev.h`; mappings are in
`main/keyer_pin_settings_esp32_dev.h`.

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

With GPS enabled, startup loads saved settings and initializes the display
before opening the GPS UART and showing its baud-rate message. Drawing this
message before TFT/SPI initialization could previously stop boot with no LCD
activity or usable USB ports. MCP23017 does not disable the composite USB
interfaces. For the current Pico display profile, GPS RX is GPIO5 (UART1,
38400 baud), GPS TX is disabled, and MCP23017/WM8960 share I2C0 on GPIO20/21.
MCP23017 address is 0x27 and its interrupt input is GPIO22. Check address
straps, 3.3V logic, common ground, and I2C pull-ups when wiring the modules.
The GPS status clock redraws only when its displayed time or status color
changes, avoiding repeated SPI transfers that can starve WM8960 tone output.
GPS parsing, clock drawing, and pending grid processing pause while PTT or
key-down is active, including GPS running on core 1. Pending grid updates
resume when idle. The UART can still receive into its FIFO; prolonged
transmission may overflow that buffer, with later NMEA sentences restoring GPS.
Clock and location status are independent: fresh valid time keeps the clock
active even without a position fix. Grid status requires fresh valid location;
data becomes stale after five seconds without an update. Raw UART bytes alone
do not mark either status active. A stale fix only dims the grid; the last GPS
grid is kept for 60 s before switching to the fallback, so brief indoor
dropouts do not rewrite flash. Without location, the configured fallback
grid remains visible; a GPS fix produces an eight-character grid in the status
row (the separate saved-grid digit setting controls memory storage). A saved
grid change plays a short 60 ms 300 Hz confirmation beep, at most once every
5 minutes so a position near a grid border does not beep repeatedly (the grid
itself is still updated and saved). On the Pico WM8960 build every tone,
including CW sidetone, has a 3 ms smoothed rise and fall to avoid clicks.

### Pico WM8960 rotary encoder

`FEATURE_ROTARY_ENCODER` is enabled in the Pico configuration. The user's
pin assignments in `main/keyer_pin_settings_pico.h` are **CPU GPIO numbers**,
not MCP pin numbers:

| MCP23017 feature | Encoder A/B | Switch |
|---|---|---|
| Enabled | GPIO2 / GPIO3 | GPIO4 |
| Disabled | GPIO22 / GPIO14 | GPIO42 |

GPIO42 applies only to RP2350B boards; the RP2350A/Pico2-W direct-GPIO
profile uses GPIO28 for that switch.

These conditional assignments reuse CPU pins freed by moving key inputs
and TX/PTT outputs onto the MCP23017. The current direct-GPIO profile disables
TX/PTT GPIO outputs (zero) to free GPIO14/22. Internal pull-ups are enabled;
connect encoder contacts/switch to ground. Review assignments before changing
features or restoring direct TX/PTT pins.

With a switch, the initial function is MIDI VFO tuning. All controls initially
belong to encoder 1, in this linear order:

**VFO, master volume, Local Volume, sidetone volume, sidetone frequency,
CW speed, USB microphone level, output route, RX threshold, RX frequency, RX WPM.**

**Local Volume** (command/assignment name `LOCALRX`) adjusts only the WM8960 USB-playback listening level in every
MIDI mode. It uses the existing saved local playback level/mute, independently
of PC volume in POD mode. Leave PC output high enough for decoding and turn
Local Volume down for comfortable speaker/headphone listening; sidetone and
microphone levels are unaffected. The decoder taps raw USB PCM before this
gain/mute, so Local Volume can even be muted without stopping decoding.
Master retains its existing behavior: PC volume in POD, local playback in
other modes (where it and Local Volume adjust the same local level).
Use `\:LOCALRX 0..100`, `+`, `-`, `MUTE`, `UNMUTE`, or `TOGGLE`, or assign
it with `\:ENCODER 1 LOCALRX`. Old saved menu files are migrated without
discarding assignments/RX settings; Local Volume is added to encoder 1.

- First press opens a single horizontal strip in the CW text area. The
  current control is enlarged and centered, with the previous/next assigned
  controls shown in smaller boxes on its left/right. Selection scrolls the
  strip through that encoder's assigned controls in a wrapping linear cycle.
  Rotation wraps in both directions through VFO without closing the menu.
  Selecting VFO by touch or confirming an already highlighted VFO with the
  switch closes it. The encoder then tunes VFO without
  displaying an adjustment popup.
- **Select:** rotation selects the next/previous control with wrap-around,
  without changing its value. Two decoded encoder pulses are required per
  selection change; reversing direction cancels an unfinished move.
  `ENCODER_MENU_SELECTION_PULSES` defaults to 2 and initializes each
  `PicoEncoderControl::selection_pulses` attribute. Adjustment sensitivity
  is unchanged. Press again to enter **Adjust**.
- **Adjust:** rotation changes the highlighted control. Another press
  returns to Select. Its highlighted background and range bar are inside
  its label box. Relative controls, including VFO, have no range bar because
  they do not provide an absolute setting/position.
- Every label box shows its current value: percentages, Hz, WPM, output
  route, RX threshold (`0 Off`) or automatic RX speed (`Auto`). Values
  use bitmap fonts (not the scroll area's large free font): the centered
  control's value is enlarged, while side controls retain their small font.
  Values refresh while the menu is open, including changes from other controls.
- Tap a label to select it directly in Adjust mode. Drag left/right to
  decrease/increase its setting; a drag remains attached to the starting
  label instead of jumping to other controls it crosses.
- Hold the switch for one second, tap **Close**, or leave the menu idle
  for five seconds to close it. Release after a long hold does not reopen it.
  Every decoded movement resets the timer, including the first of two
  selection pulses and adjustments at a value limit.
  CW scroll text is restored without clearing its buffer.
- Rotation with the menu closed adjusts the last-selected control as before.
  A change shows a centered copy of that control's label box for one second;
  further changes refresh its value and restart the timeout. It uses the
  same small font, value and (absolute controls only) range bar as the menu.
  No separate box appears while the full menu is open. The legacy text-only
  encoder messages remain disabled on TFT builds; error notifications remain.

Each entry in `encoder_control_attributes` in `main/main.cpp` provides
`label`, `relative` and `popup_enabled`. VFO has popup disabled; other controls
initially allow the centered box. Set `popup_enabled=false` on any entry to suppress it without
disabling adjustment or its full-menu box. Set `relative=true` for a
relative-only control so neither view draws a misleading range bar.
Touch dragging is restricted to the selected control in an active menu or a
visible, unexpired popup. Closing a view ends its drag; invisible controls cannot
be adjusted by touching their former location. Encoder rotation continues
adjusting its current selection until a new selection is made.

On Pico WM8960 builds, assignments, each encoder's selected control and
selection sensitivity, popup flags, output route and RX settings survive reboot.
The transient open menu, Select/Adjust state and touch gestures are not saved.
Existing volume, sidetone pitch and CW speed still use their existing EEPROM
settings. A separate versioned/checksummed LittleFS file stores the new settings,
without changing the EEPROM layout or CW memory addresses.

Select **4MB (Sketch: 4032KB, FS: 64KB)** in the Arduino Flash Size menu
(CLI board option `flash=4194304_65536`, alongside `ipbtstack=ipv4btcble`).
Keep the same flash/partition selection in future builds to retain the file.
The first boot initializes only a completely blank filesystem partition;
a nonblank partition that cannot mount is never automatically formatted.
Mount/load/save failures are reported on CDC2. Invalid files are retained
without overwriting; settings stay runtime-only until storage is repaired.
Writes are coalesced after two seconds without changes or encoder movement,
only with the keyer idle, and use the existing audio pause/resume protection.
Failed writes retry no more than once per five seconds; a temporary file is
verified before atomic replacement. Wait at least five seconds idle before
powering off after changing settings. Saved assignments to disabled encoders
are retained and reported at startup, so changing wiring/build options does not
silently discard them.

The switch has 20 ms debounce. Menu drawing is bounded to one label per
25 ms display pass, with the selected label prioritized. PTT or key-down
pauses encoder processing, mode-icon drawing, and messages. Captured steps
remain queued and resume when idle (subject to the existing queue capacity).
Encoder value messages expire one second after the last displayed update;
expiry is serviced while idle even outside normal keyer mode and does not
wait for an in-progress scroll redraw. The temporary BT saved-address and
stack-start/ready startup diagnostics have been removed; pairing prompts
and errors remain.
WM8960 feedback beeps finish with audio servicing before their callers
draw LCD messages. Normal display servicing pauses during local or MIDI
tones, key-down, or PTT, except that the cached T/R icon updates even during
held tune/key-down. During PTT the CW text box also updates
(between elements, at most two characters per pass), so paddle,
straight-key and keyboard text appear as it is sent. On TFT displays the CW
text box draws only new characters and repaints scrolled rows incrementally.
Typed keyboard CW uses the paddle PTT hang/tail policy; the T/R icon follows
the actual PTT state. Serial/WinKey text and memory playback retain their
automatic-send PTT timing.
Pico WM8960 builds clear one row in a separate pass and draw at most two
characters per pass, including row repaints, so a full row cannot monopolize
foreground decoder servicing. GPS processing also pauses during feedback tones.
Touch, keyboard, and terminal command polling must remain available during
tune/TX so stop commands can be received; audio servicing takes priority
over GPS and LCD work. Pico WM8960 resistive touch reads use two samples
three milliseconds apart with continuous audio servicing between samples,
instead of the library's repeated blocking delays. Pressure, raw-position
stability, and calibrated screen bounds are checked before accepting a touch.
Feedback-tone waits service audio continuously without millisecond sleeps.
They wait for generated tone samples to finish, then service the six-millisecond
DMA tail before LCD drawing. A stalled tone is stopped with an explicit error
after its requested duration plus one second, rather than hanging the caller.
VFO uses only the status letter, without a per-step LCD message.
The status row immediately to the right of WPM shows a light-green mode letter:
**V** = VFO, **M** = master volume, **S** = sidetone volume,
**F** = sidetone frequency, **W** = CW speed, **U** = USB microphone level,
**O** = output routing, **D** = RX threshold, **R** = RX frequency,
**C** = RX decode WPM.

Setting `rotary_switch` to **0** disables that switch's setup/reads and starts
encoder 1 on **master/USB playback volume**, not speed. A runtime assignment
can select a different control even without a switch.
Menu selection/assignment, output routing and RX controls are not yet saved.
Existing audio/pitch/speed settings retain their deferred EEPROM saving,
with no layout or magic-number change.

Volume functions use the same linear 2% steps and
unmute the selected path. Master volume changes the existing saved USB playback
trim, not the codec's analogue output gain or sidetone volume. Frequency
changes by 10 Hz per decoded step, limited to 300..1200 Hz. CW speed retains
the existing 1/2-WPM acceleration, command-mode speed handling, WinKey speed
notification, LCD update, and normal-speed MIDI CC21 synchronization.
VFO emits one CC20 relative message per decoded step (positive 127, negative 1).
`ENCODER_VFO_DIRECTION` in `main/keyer_settings_pico.h` defaults to **-1**
to reverse VFO tuning; set **+1** for the original direction. The saved
`\:MIDI-VFO NORM|REV` terminal command reverses the direction at runtime
without rebuilding (this saved setting and `\:MIC GATE` bumped the Pico EEPROM
magic to 47, so the first boot resets saved settings). Other modes
are unaffected.
The graphical Select/Adjust menu requires a TFT. Non-TFT builds retain
short-press function cycling and long-press return to the first assigned
control, with their existing value messages.
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

#### Additional encoders and pin budget

With MCP23017 enabled, CPU GPIO26/27 and switch GPIO28 can support encoder 2
without removing any of the six transmitter pairs. It is **disabled by
default**: set `PICO_ENCODER2_ENABLED=1` in the Pico pin configuration or
compiler definitions when wired. Encoder 1 remains GPIO2/3, switch GPIO4.
GPIO14 is reserved as `PICO_COMMAND_BUTTON_GPIO` for a future digital
Command-mode button; this chunk does not implement that button.

The live terminal command `\:ENCODER` lists bindings. For example:

```text
\:ENCODER 2 MASTER
\:ENCODER 2 OUTPUT
\:ENCODER 1 ALL
```

A control has exactly one owner. Assigning it moves it from its previous
encoder; all controls initially belong to encoder 1. The recipient selects
the transferred control immediately. Multiple controls on a switchless
encoder can still be selected through subsequent assignments or a future
menu-launch touch/button action.

Slots 3/4 default to disabled (`PICO_ENCODER3_A/B/SWITCH` and
`PICO_ENCODER4_A/B/SWITCH` are -1). RP2350B boards expose additional GPIOs
that can be assigned after checking their physical wiring. The standard
Pico2-W with this TFT/touch/audio/GPS wiring does not have enough spare
header GPIOs for four three-contact encoders while retaining the reserved
button and UART0 pair. Startup validation rejects overlaps with the configured
display, audio, GPS, keying, encoder and reserved button pins.

The current MCP23017 map uses three key inputs and twelve transmitter
outputs, leaving only B7 free. A three-contact expander encoder would need
at least one transmitter pair removed; more expander encoders would require
further reductions. No expander encoder backend is enabled: the available
CPU interrupt pins avoid I2C edge-loss risk and preserve all six TX pairs.

The direct-key/no-MCP profile reserves GPIO26 instead; encoder 2's default
26/27/28 wiring must be reassigned before enabling it there.

`open_encoder_menu(index)`, `encoder_menu_switch(index)` and
`close_encoder_menu()` are the shared foreground hooks for a future touch
launcher or standalone switch (index is 0..3). They do not require an encoder
rotation to operate the touch menu.

Run `.\tests\wm8960_midi_host_test.ps1 -CwEncodersOnly` for the targeted
actual-source routing, binding, switch, Select/Adjust, highlight, touch drag
and timeout tests.
Pico2-W with encoder 2 enabled and RP2350B builds pass. Physical encoder/menu
and touch-drag testing remains pending.

### Pico USB RX CW decoder: first hardware-test chunk

The decoder consumes one channel of signed stereo 48 kHz USB playback
before local/USB device playback trim, mute, output routing or local/MIDI
sidetone mixing. It does not decode the microphone or locally generated
sidetone. Route the receiver's audio to the keyer's USB speaker endpoint
from the PC. A PC application that mutes or stops supplying PCM cannot
be bypassed by the decoder tap.

This adapts the streaming detector design from
`RP2350-SDR-CW-Interface`: 48-to-8 kHz averaging, 20 ms Goertzel windows,
5 ms hops, hysteresis and debounce, with bounded foreground DSP rather
than launching another core-1 task. Timing comes from samples, not delayed
foreground wall-clock intervals. Fixed speed is available, with automatic
learning from marks and actual inter-element gaps. Stream gaps, overflow,
USB alternate-setting changes and receive-setting changes reset partial
decodes instead of joining unrelated audio.

Decoded text goes to the CW text box and **CDC2 terminal only**, never the
primary WinKey CDC channel. PCM/text drops are counted and reported on
CDC2 when space is available; decoder work never waits for terminal output.
Received glyphs use `WM8960_RX_TEXT_COLOR` from the Pico feature configuration,
default **TFT_CYAN**; local sent text retains white. Colors follow individual
characters through scrolling and redraws. Each decoded character/word space
resets the backlight inactivity timer and wakes the TFT backlight.

```text
\:OUTPUT BOTH
\:RXFREQ 600
\:RXWPM 20
\:RXCHANNEL 0
\:RXTHRESHOLD 20
\:RX
\:RXTHRESHOLD 0
```

- `OUTPUT SPEAKER|HEADPHONE|BOTH|NONE` powers the corresponding codec
  output stages. It affects USB playback and generated tones alike, but
  does not disable capture, USB endpoints or receive decoding.
- `RXTHRESHOLD 0` stops PCM decimation/DSP, not just text output.
  **1..100: higher is stricter**. This sensitivity control is not a percent
  audio volume; its tone-amplitude floor ranges approximately -66..-26 dBFS,
  with an additional adaptive noise threshold.
- `RXFREQ 300..1200` sets the saved receive center, independent of sidetone
  frequency. Automatic tone tracking searches within +/-40 Hz of this center,
  clipped to 300..1200 Hz. The menu continues to show the center; `RX` also
  reports the actual tracked frequency and switch count since the last
  stream/settings reset.
- `RXWPM 5..60` sets fixed speed; `0` enables automatic learning. Send a
  short training sequence such as repeated V characters before judging
  auto-speed text. It does not measure carrier-frequency offset.
- `RXCHANNEL 0|1` selects left/right. Channels are not summed, avoiding
  cancellation when the receiver supplies opposite-phase stereo.
- `RX` displays settings, signal/noise amplitude and drop counters.

Defaults in `main/keyer_features_and_options_pico.h` are RX disabled,
600 Hz, fixed 20 WPM, left channel and both physical outputs. Overrides:
`WM8960_RX_THRESHOLD`, `WM8960_RX_FREQUENCY`, `WM8960_RX_WPM`,
`WM8960_RX_CHANNEL`, `WM8960_OUTPUT_ROUTE`. These are first-boot defaults;
valid saved menu settings take precedence. See the LittleFS partition and
persistence requirements above; existing EEPROM CW memories are preserved.
The legacy analog/digital `FEATURE_CW_DECODER` is separate and need not be
enabled for USB decoding.

Run `.\tests\cw_rx_host_test.ps1` for exact synthetic SOS output from
5..60 WPM, 300..1200 Hz, either stereo channel, auto learning after training,
stricter threshold rejection, white-noise/off-frequency rejection, disable
and overflow/frequency-reset recovery. Fixed-speed SOS is checked at every
integer speed from 5 through 60 WPM. Additional cases cover short bursts,
interrupted dahs, spacing after rejected pulses and Auto learning immunity
to rejected bursts.

Once speed is fixed or Auto has acquired four marks, pulses shorter than
half the estimated dit (with a 15 ms minimum) are normally rejected.
A short dit of at least 15 ms can be rescued after an accepted dah inside
a character, with a preceding gap of 0.5..1.75 dits and strong tone evidence
in at least two 5 ms hops: spectral amplitude above 1.15 times broadband RMS
and eight times the estimated noise. Isolated short bursts are not rescued;
neither rescued nor rejected pulses enter speed learning.
Key-up confirmation scales with dit length, from
10 to 15 ms, bridging brief detector dropouts inside marks. At 27 WPM this
rejects 20 ms detector marks and bridges the observed 10 ms dah dropout.
Confirmed edges retain their first-candidate sample timestamps, and rejected
pulses remain part of the surrounding gap rather than restarting spacing.
Auto acquisition initially retains the 15 ms minimum and 10 ms confirmation
so a slow initial estimate does not reject genuine fast-speed training.
Longer noise bursts can still look like legitimate Morse elements.

After fixed-speed selection or Auto acquisition, a mark beginning after at
least five dits of silence (and at least 200 ms) is provisional if it lacks
two consecutive high-quality 5 ms hops. These require spectral amplitude
above eight times estimated noise and 1.25 times broadband RMS; below a
30 ms dit, the RMS factor is 1.15 to preserve fast keyed-window edges.
A provisional lone E/T is discarded at the character boundary and excluded
from speed learning. If another accepted element follows before that boundary,
the weak first element is retained, avoiding lost onsets in multi-element
characters. In-character hold detection and the short-dit rescue are unchanged.
This reduces pause noise, but can also suppress weak genuine isolated E/T
characters; noise forming multiple elements can still pass.

Frequency tracking is enabled automatically. A 5 Hz Goertzel search grid
requires four supported 5 ms hops, with spectral amplitude above 1.15 times
RMS and at least six times estimated noise (minimum 1000 PCM amplitude).
A 20 Hz switching deadband avoids small pitch jitter. The tracker holds its
lock through brief fades/gaps and permits reacquisition after 150 ms without
credible matching tone. It returns to the selected center on stream reset,
without changing saved settings, WPM, sidetone or encoder assignments.
Overlapping signals, first-element acquisition and weak callers can still
produce errors. DSP remains foreground-only and uses the existing 2 ms
cooperative budget; check queue/drop and processing diagnostics on hardware.

The USB decoder estimates the background from a rolling lower envelope,
excluding coherent tone energy, even when startup noise initially trips
the detector. This prevents a loud background from leaving the noise estimate
stuck at its quiet startup value. Threshold 1..100 sets a minimum amplitude
floor and scales the adaptive noise and tone-onset peak limits: 1 is about 0.63 times,
50 preserves the baseline, and 100 is 1.6 times. Higher values are stricter;
0 disables decoding. The tone-coherence requirement remains independent.
The peak hold limit remains unchanged to preserve fast keyed-edge timing.
The adaptive limit may exceed the amplitude floor. Real noisy recordings
can still produce timing errors; raising the threshold alone is not a cure.
Tone detection uses a 20 ms spectral window with 5 ms hops and a decaying
signal-peak gate to separate the keyed tone from broadband background.
The peak decays by 2% per hop (about 172 ms half-life), allowing quieter
letters after louder ones instead of holding the old amplitude gate for
multiple characters. Host tests cover a sudden amplitude reduction between letters.
Tone onset also requires spectral amplitude above the window RMS, with a
lower 0.65-times-RMS hold limit to avoid chattering at keyed edges. This
rejects broadband bursts and nearby off-target tones more strongly; tune RX Hz
to the station's actual pitch. Weak or overlapping stations can still lose marks.
Auto timing changes require supported mark-history estimates; a single short
key-up gap no longer directly changes the dit estimate.

For local recording regression, `cw_rx_host_test.ps1` accepts
`-RecordingPcm <path> -ExpectedSubstring <text>` in addition to its synthetic
tests. Supply headerless signed 16-bit little-endian stereo PCM at 48 kHz;
the replay uses channel 0, 700 Hz, threshold 50 and fixed 20 WPM.
It checks zero PCM drops and the specified reference substring, not full
transcript accuracy. User recordings stay outside the repository.
`RX` also reports queued samples, the maximum decoder-service gap, maximum
processing time, and the longest instrumented foreground stage (including
serial, touch, keyboard/GPS, settings, display and BOOTSEL). These maxima are
cumulative since boot; reboot after flashing to compare a new build.
Processing catches up with at most 1280 samples (160 ms of
8 kHz input) per call and checks a 2 ms wall-clock budget after each DSP window.
Cooperative `myDelay` waits also service decoding on core 0, with a reentry
guard; decoded text defers scroll-box clearing to display service instead of
performing a full TFT clear inside DSP. Main-loop stage boundaries also service
decoding. A slow-display host regression models 40 ms glyph/row-clear calls
and verifies at most 80 ms per incremental renderer pass with zero PCM drops.
This bounds that renderer, not every potentially blocking foreground call.
Pico WM8960 full scroll-box clears use eight-pixel strips with RX servicing
between TFT transactions; twelve-pixel rounded end caps preserve the corners.
This reduces uninterrupted clear time without changing the queue capacity.
Encoder menu clears and control-box backgrounds likewise fill in eight-pixel
strips, with RX servicing between fills, labels and queued encoder actions.
This keeps RX WPM adjustments between stations cooperative.
CDC2 terminal writes also service RX outside the USB mutex while waiting
for host buffer space; their existing one-second no-progress timeout and
write-error reporting remain unchanged. A host test applies 600 ms of CDC
backpressure and checks zero PCM drops. Foreground timing separates serial
handling from send-buffer/PTT and potentiometer/encoder work.
The queue holds 512 ms. Growing PCM-drop
counters indicate queue overflow and discarded partial characters, separately
from recognition errors. Counters/high-water timing values are cumulative
since boot, so compare successive readings rather than one snapshot.

`.\tests\wm8960_midi_host_test.ps1 -CwEncodersOnly` also exercises decoded
text through the TFT incremental renderer beyond three full boxes, including
word-space wrapping and appending after an empty-buffer repaint.

Hardware acceptance order:
Small-font control values and headphone/speaker switching have been
hardware-confirmed. The remaining checks below are still required.

1. Check SPEAKER, HEADPHONE, BOTH and NONE using a quiet test tone; verify
   sidetone, microphone capture and both serial channels still work.
2. Send clean known 20-WPM/600-Hz CW over USB; confirm CDC2/display text and
   no unsolicited WinKey bytes. Adjust RX threshold; verify 0 stops decoding.
3. Test the encoder menu, direct label taps, drag direction, timeout and
   per-encoder assignments with encoder 2 wired; then test real noisy RX audio.
4. Change the selected control, assignments, output route and RX settings;
   wait five seconds idle, power-cycle and verify restoration and unchanged
   CW memories. Receiver-noise tuning follows this hardware checkpoint.

### Pending Pico integration worklist

All items below are pending; order follows the requested worklist.

1. Repair or replace the suspected damaged touch-panel membrane, then
   retest touch. Only occasional events occur when pressure is applied at
   the left edge; physical touch testing, including stop-TX/tune controls,
   is blocked until working hardware is available.
2. Test Command mode using a single button.
3. Add a touch launcher for the new audio/decoder menu (and a dedicated
   mute control), usable without a serial terminal, keyboard or encoder switch.
4. Port this project's Pico Bluetooth keyboard support into
   `RP2350-SDR-CW-Interface`.

Hardware testing confirms clean paddle/keyboard CW, Ctrl-T tune, and
Ctrl-O feedback tones in On, Paddles Only, and Off modes. The audio-serviced
touch implementation has host-test coverage but is not hardware-verified.

- Mike K7MDL

********************************
*****************************************
