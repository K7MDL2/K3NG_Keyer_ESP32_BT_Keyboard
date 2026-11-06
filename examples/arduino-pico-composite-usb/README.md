# Arduino-Pico composite USB examples

Two standalone Arduino sketches show how a Pico or Pico 2 can appear to a PC as a
single USB device with:

- two serial (CDC) ports,
- a USB MIDI port,
- USB audio playback and capture (UAC2, 48 kHz, 16-bit, mono).

They use the techniques from the K7MDL fork of K3NG CW keyer on the Pico WM8960 build, without the
rest of the keyer. Use them to learn how the USB setup works, test a board, or
start your own project. They are not part of the keyer firmware build.

| Sketch | Hardware | What you get |
| --- | --- | --- |
| [CompositeUSB](CompositeUSB/CompositeUSB.ino) | Any Pico (RP2040) or Pico 2 (RP2350); nothing else | Serial, MIDI, and USB audio with a test tone. PC playback is accepted but discarded. |
| [WM8960USB](WM8960USB/README.md) | Pico 2 / Pico 2 W (RP2350) plus a WM8960 codec module | Real audio: PC playback plays on the codec output, and the codec microphone is sent to the PC. |

Both sketches work with the stock Arduino-Pico core. You don't need to install
libraries or edit core files.

> Flashing an example replaces the keyer firmware on that board. Reflash the
> keyer afterwards, or use a spare board.

## Requirements

- Arduino-Pico core (tested with **6.3.2**).
- **USB Stack: Pico SDK** (the default). Do not select Adafruit TinyUSB; the
  sketches stop with a compile error if it is selected.

## Build

Open the `.ino` file in Arduino IDE, select your board, and upload. Or use
`arduino-cli` from this folder:

```powershell
arduino-cli compile --fqbn rp2040:rp2040:rpipico   --output-dir .\build-rp2040 .\CompositeUSB
arduino-cli compile --fqbn rp2040:rp2040:rpipico2w --output-dir .\build-pico2w .\CompositeUSB
arduino-cli compile --fqbn rp2040:rp2040:rpipico2w --output-dir .\build-wm8960 .\WM8960USB
```

## What appears on the PC

| Interface | CompositeUSB | WM8960USB |
| --- | --- | --- |
| Serial port 1 (CDC0) | Command-line interface and status | Same |
| Serial port 2 (CDC1) | Echoes typed text; receives `\text` output | Same |
| USB MIDI | Echoes incoming packets; sends test Note/CC messages | Same |
| Playback device | Accepts audio and discards it | Plays on the WM8960 headphone/speaker output |
| Recording device | Silence, or a 1 kHz test tone on request | WM8960 microphone, plus the optional test tone |

Port order on the PC isn't guaranteed. Use the interface names to tell the ports
apart.

## Command-line interface

Open **serial port 1** at 115200 baud. Commands end with Enter (CR, LF, or CRLF
line endings). Type `\?` to list the commands. Turn on local echo in your
terminal if you want to see what you type.

| Command | Result |
| --- | --- |
| `\?` | List commands and what each one does |
| `\text` | Send `Composite USB sample text: hello from Pico!` to **serial port 2** (open a second terminal on it) |
| `\note` | Send MIDI channel 1 Note On, note 60 (middle C), velocity 100; Note Off follows 500 ms later |
| `\cc` | Send MIDI channel 1 CC21, value 64 |
| `\tone` | Play a 1 kHz test tone for three seconds on the recording device (and on the codec output in WM8960USB) |
| `\stop` | Stop the test tone |
| `\status` | Print USB, audio, and error counters |

Commands are lowercase, and unknown commands print an error. If the PC isn't
reading the port, text and MIDI sends time out after five seconds.

### Tips

- **Hearing the test tone on PC speakers:** the tone goes to the PC's
  *recording* device. To hear it, open the recording device at 48 kHz/16-bit in
  a recorder or DAW with input monitoring on. On Windows, you can instead turn
  on **Listen to this device** in the recording device's properties. Start with
  the volume low.
- **MIDI:** open the MIDI input in a MIDI monitor before sending `\note` or
  `\cc`. Turn off MIDI thru/echo in the PC application, or the sketch's echo
  can loop. To hear the note, you need a software synthesizer.
- **`\status` counters:** `OUT`/`IN` are USB audio bytes. `CDC2` counts bytes
  echoed on serial port 2. `MIDI` counts echoed MIDI packets. `ALT` counts
  audio stream starts and stops. `ERR` counts USB write failures and should
  stay at 0.
- The 1200-baud reset into BOOTSEL still works on serial port 1, as on any
  Arduino-Pico sketch.

## How it works

The Arduino-Pico core ships with one serial port and no USB audio. These
sketches add the extra functions from inside the sketch, using local copies of
a few TinyUSB driver files:

1. [composite_config.h](CompositeUSB/composite_config.h) includes the core's
   TinyUSB configuration, then raises the counts: 2 CDC, 1 MIDI, 2 audio
   functions. It also sets the audio buffer sizes.
2. [overlay_cdc.c](CompositeUSB/overlay_cdc.c) builds the CDC driver with two
   ports. Only port 1 line-state and baud-rate events reach the core's
   `Serial`, so opening or closing port 2 doesn't affect port 1.
3. [overlay_midi.c](CompositeUSB/overlay_midi.c) builds the real MIDI driver in
   place of the core's empty stubs.
4. [overlay_audio.c](CompositeUSB/overlay_audio.c) builds the USB audio driver
   and registers it with the core as an application driver.
5. [overlay_dcd.c](CompositeUSB/overlay_dcd.c) builds the RP2040/RP2350 USB
   hardware driver with one fix: an audio stream can be restarted without a
   "endpoint already available" panic. The fix is the one merged upstream in
   [hathach/tinyusb#2937](https://github.com/hathach/tinyusb/pull/2937).
6. [composite.cpp](CompositeUSB/composite.cpp) briefly disconnects USB,
   allocates extra endpoints, registers the audio, serial, and MIDI interfaces
   with `USB.registerInterface`, then reconnects. The audio, CDC, and MIDI
   callbacks and the command-line interface are also here.

The overlay files `#include` the TinyUSB sources that ship with the installed
core, so no TinyUSB code is copied into this folder. The sketch's versions
replace the core's prebuilt ones when the program is linked. Because of this,
the method depends on the core version. After updating Arduino-Pico, rebuild
and retest.

The core's USB task still runs TinyUSB. Serial port 2 and MIDI writes from
`loop()` take the core's `USB.mutex` first. Audio callbacks already run inside
the USB task, so they don't take the mutex again.

## Tests

Two PowerShell scripts compile the sketch logic on Windows with Visual Studio
C++ build tools and check it with simulated USB calls. They don't need a board.

- `.\test-cli.ps1` checks CompositeUSB: command parsing, serial-port-2 text,
  exact MIDI bytes, Note Off timing, timeouts, and the tone output.
- `.\test-wm8960.ps1` checks WM8960USB: feedback direction and limits,
  playback rendering and mute, tone level and fades, buffer overflow, and
  capture behavior when the codec runs out of data.

## Status

Both sketches are tested on Windows:

- **CompositeUSB**: serial, MIDI, and audio working.
- **WM8960USB** (Pico 2 W):
  - clean PC playback to the codec output,
  - microphone heard on the PC,
  - clean test tone.

Linux and macOS haven't been tested.

## Credits

- USB audio, CDC, and MIDI composite work originated in PA3GSB's
  [RP2350-SDR-CW-Interface](https://github.com/pa3gsb/RP2350-SDR-CW-Interface)
  ([K7MDL2 fork](https://github.com/K7MDL2/RP2350-SDR-CW-Interface)).
- Arduino-Pico adaptation: this keyer project. See `main/wm8960_audio.cpp`,
  `main/wm8960_tinyusb_config.h`, and the `main/wm8960_tinyusb_*.c` wrappers.
- Audio stream-restart fix:
  [hathach/tinyusb#2937](https://github.com/hathach/tinyusb/pull/2937).
- TinyUSB sources included by the overlays are MIT-licensed and keep their
  original notices.
