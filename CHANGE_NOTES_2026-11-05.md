# November 5, 2026 - Pico WM8960 receive decoder and encoder controls

## Highlights

- Added CW receive decoding entirely on the keyer for Pico builds with WM8960 USB audio. No PC decoding service is required.
- Added a four-encoder assignment framework, a touch-enabled encoder menu, and persistent receive/menu settings.
- Added independent Local Volume control for listening without reducing the audio supplied to the decoder.
- Reduced foreground stalls during display updates, encoder adjustments, and terminal backpressure.

## CW receive decoding

- Decode the selected left or right USB speaker PCM channel before local playback gain/mute. Stereo channels are not summed.
- Display received text in cyan by default, alongside locally sent text, and send receive text to CDC2 only. The primary WinKey CDC channel is not used for received text.
- Provide threshold, receive center frequency, fixed/automatic WPM, and channel controls:

  ```text
  \:RXTHRESHOLD 76
  \:RXFREQ 700
  \:RXWPM 27
  \:RXCHANNEL 0
  \:RX
  ```

- Threshold 0 disables receive DSP. Values 1..100 select an amplitude floor and adaptive detection sensitivity; higher values are stricter.
- Support receive centers from 300 through 1200 Hz and fixed speeds from 5 through 60 WPM. RXWPM 0 enables automatic learning.
- Automatically track tones within +/-40 Hz of the selected center, bounded to 300..1200 Hz. The selected center remains the saved/menu value.
- Require supported tone evidence before changing frequency, use a 20 Hz switching deadband, and retain a lock until 150 ms without credible matching tone.
- Report tracked frequency and switch count separately in RX status.
- Improve fading response, coherent-tone discrimination, short-dropout bridging, and short-noise-mark rejection.
- Rescue short, credible dits after a dah without feeding clipped durations into automatic speed learning.
- Treat low-quality isolated marks provisionally: retain a weak onset if another element follows within the character, but reject an unsupported lone mark.
- Reset partial decoding on stream discontinuities, overflow, USB lifecycle changes, or receive reconfiguration.

## Encoder menu and audio controls

- Support up to four configured encoders with independently assigned controls. Additional encoders require valid, nonconflicting GPIO assignments.
- Available controls: VFO, Master Volume, Local Volume, Sidetone Volume, Sidetone Frequency, CW Speed, Microphone Level, Output Route, RX Threshold, RX Frequency, and RX WPM.
- Use a wrapping Select/Adjust menu with current values, enlarged selected control, adjacent controls, and range indicators where applicable.
- Support touch selection and dragging, long-press closure, idle timeout, and centered adjustment popups.
- Default menu selection sensitivity is two decoded encoder pulses; adjustment sensitivity is unchanged.
- Local Volume affects only WM8960 USB playback listening level/mute. In POD mode, Master Volume continues controlling the PC.
- Local playback mute does not stop receive decoding.
- Select speaker, headphone, both, or neither codec output without disabling USB audio capture or receive decoding.

## Settings persistence

- Save encoder assignments/selections, selection sensitivity, popup preferences, output route, and RX settings in a versioned, checksummed LittleFS record.
- Preserve existing EEPROM volume, speed, sidetone, and CW memory layouts.
- Migrate earlier menu records while preserving assignments and receive settings.
- Coalesce saves while idle, pause audio around flash operations, verify temporary writes, and replace the settings file atomically.
- Initialize only a completely erased filesystem partition. Nonblank partitions that fail to mount are not automatically formatted.
- Report storage failures on CDC2 rather than silently overwriting settings.

**Pico build requirement:** retain the 4 MB flash / 64 KB filesystem selection
(`flash=4194304_65536`). Changing partition layout can invalidate stored settings.
Allow at least five seconds idle after changing settings before powering off.

## Servicing and diagnostics

- Bound Pico WM8960 TFT append and repaint work to two characters per pass; preserve legacy text-LCD behavior.
- Split large scroll/menu/popup clears into smaller fills with receive servicing between operations.
- Service receive decoding between foreground stages, encoder actions, and labels.
- Service receive decoding during CDC2 write backpressure outside the USB mutex, retaining existing timeout/error behavior.
- Report PCM/text drops, pending queue samples, maximum receive service gap, maximum processing time, and longest foreground stage.
- A foreground stage's total duration can include cooperative receive servicing; it is not necessarily an uninterrupted decoder stall.

## Other included changes

- Name the TFT_eSPI `setup_t` structure for compatibility.
- Select the M5Stack Core2 ESP32 development display profile and enable BLE in that profile.
- Disable ESP32 bootloader logging in the checked-in configuration.
- Include the pending updates to the tracked Hosyond precompiled application and bootloader images.

## Validation and limitations

- RP2350B/Waveshare RP2350B Plus W build succeeds using repository component libraries, Bluetooth enabled, and the 64 KB filesystem layout.
- Host tests cover fixed/learned speed, stereo selection, detection thresholds, fading/noise, short marks, stream recovery, frequency handovers, and search-boundary frequencies.
- Fixed-speed tests include every integer speed from 5 through 60 WPM. Isolated E/T tests cover that range at multiple frequencies.
- Six focused encoder/audio/display integration configurations pass.
- Production frequency tracking matches offline recording experiments; the mixed-caller reference recording makes 14 switches and recovers repeated AB4PP exchanges with zero replay PCM drops.
- Hardware observations before the final publication showed zero PCM/text drops and maximum receive service gaps around 180 ms. These are observations, not universal timing guarantees.
- Noisy, overlapping, fading, or irregular hand-sent CW still produces errors. Weak isolated E/T characters may be suppressed, and first-element acquisition may miss part of a caller.
- Fixed WPM matching the sender remains recommended when Auto learning becomes unstable.
- Tracking adds foreground DSP work. Monitor processing-time and queue/drop diagnostics on hardware.
- Automatic letter/word-gap experiments and DeepFist neural decoding research were **not applied**. Decoding remains entirely on the keyer.

This release does not change the existing USB audio, MIDI, or dual-CDC endpoint layout.
