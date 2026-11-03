#if defined(ARDUINO_ARCH_RP2040)
#include "keyer_features_and_options_pico.h"
#include "keyer_debug.h"

#if defined(FEATURE_WM8960_USB_AUDIO)
#include "wm8960_midi.h"
#include "wm8960_audio.h"
#include "wm8960_tinyusb_config.h"

#include <CoreMutex.h>
#include <USB.h>
#include <tusb.h>
#include <cstdio>
#include <cstring>

namespace {
constexpr uint8_t kChannel = 9;  // MIDI channel 10 is zero-based on the wire.
constexpr uint8_t kKeyNote = 17;
constexpr uint8_t kPttNote = 18;
constexpr uint16_t kQueueSize = 128;
constexpr uint32_t kQueueTimeoutMs = 250;
constexpr uint32_t kEchoWindowUs = 15000;
// Host feedback of a level the keyer just sent arrives ~250 ms later.
constexpr uint32_t kOwnFeedbackWindowUs = 1000000;
constexpr uint32_t kSpeedReportIntervalMs = 5 * 60 * 1000;

enum class PacketKind : uint8_t { Other, Key, Ptt };
struct Packet {
  uint8_t bytes[4];
  uint32_t queued_ms;
  PacketKind kind;
  bool down;
};
struct RecentPacket {
  uint8_t bytes[4];
  uint32_t sent_us;
  bool valid;
};

Packet queue[kQueueSize] = {};
uint16_t queue_read = 0;
uint16_t queue_count = 0;
RecentPacket recent[32] = {};
uint8_t recent_index = 0;
bool initialized = false;
bool connected = false;
bool faulted = false;
bool cleanup_pending = false;
bool cleanup_key_pending = false;
bool local_key = false;
bool local_ptt = false;
bool sent_ptt = false;
bool ptt_session_active = false;
uint16_t input_keys = 0;
uint16_t input_ptt = 0;
uint32_t fault_count = 0;
uint32_t reported_fault_count = 0;
uint32_t invalid_packets = 0;
uint32_t reported_invalid_packets = 0;
uint32_t sent_packets = 0;
uint32_t received_packets = 0;
uint32_t suppressed_echoes = 0;
uint16_t last_control_inputs[4] = {
    UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX};
uint32_t last_speed_wpm = 0;
uint32_t last_speed_report_ms = 0;
bool speed_reported = false;
int16_t host_mic_value = -1;
int16_t host_control_value[static_cast<uint8_t>(Wm8960MidiHostControl::Count)] = {
    -1, -1, -1, -1, -1};
constexpr uint8_t kHostControlNumbers[] = {7, 23, 21, 22, 3};
Wm8960MidiPttMode ptt_mode = Wm8960MidiPttMode::Off;
Wm8960MidiControlMode control_mode = Wm8960MidiControlMode::Pod;
volatile uint32_t usb_epoch = 0;
uint32_t serviced_epoch = 0;

#if defined(DEBUG_MIDI_MESSAGES)
struct MessageTrace {
  uint32_t time_us;
  uint8_t bytes[4];
  bool incoming;
};
constexpr uint16_t kMessageTraceSize = 128;
MessageTrace message_trace[kMessageTraceSize] = {};
uint16_t message_trace_read = 0;
uint16_t message_trace_count = 0;
uint32_t message_trace_dropped = 0;

void trace_message(const uint8_t *bytes, bool incoming) {
  if (message_trace_count == kMessageTraceSize) {
    ++message_trace_dropped;
    return;
  }
  MessageTrace &entry =
      message_trace[(message_trace_read + message_trace_count) % kMessageTraceSize];
  entry.time_us = micros();
  memcpy(entry.bytes, bytes, 4);
  entry.incoming = incoming;
  ++message_trace_count;
}

void print_message_trace(HardwareSerial &port) {
  if (!port) {
    return;
  }
  char line[192];
  int length;
  if (message_trace_count) {
    const MessageTrace &entry = message_trace[message_trace_read];
    const uint8_t *bytes = entry.bytes;
    const uint8_t status = bytes[1] & 0xF0;
    const bool channel_message = bytes[1] >= 0x80 && bytes[1] < 0xF0;
    const char *type = status == 0x80 ? "NOTE_OFF" :
        status == 0x90 ? (bytes[3] ? "NOTE_ON" : "NOTE_ON_ZERO/OFF") :
        status == 0xA0 ? "POLY_PRESSURE" :
        status == 0xB0 ? "CC" : status == 0xC0 ? "PROGRAM" :
        status == 0xD0 ? "CHANNEL_PRESSURE" :
        status == 0xE0 ? "PITCH_BEND" : "SYSTEM/OTHER";
    length = snprintf(line, sizeof(line),
        "[MIDI %s] us=%lu USB=%02X %02X %02X %02X cable=%u cin=%u ch=%u %s data1=%u data2=%u\r\n",
        entry.incoming ? "RX" : "TX",
        static_cast<unsigned long>(entry.time_us),
        bytes[0], bytes[1], bytes[2], bytes[3], bytes[0] >> 4,
        bytes[0] & 0x0F, channel_message ? (bytes[1] & 0x0F) + 1 : 0,
        type, bytes[2], bytes[3]);
  } else if (message_trace_dropped) {
    length = snprintf(line, sizeof(line),
        "[MIDI TRACE] dropped %lu messages (trace buffer full)\r\n",
        static_cast<unsigned long>(message_trace_dropped));
  } else {
    return;
  }
  if (length <= 0 || static_cast<size_t>(length) >= sizeof(line) ||
      port.availableForWrite() < length) {
    return;
  }
  port.write(reinterpret_cast<const uint8_t *>(line),
             static_cast<size_t>(length));
  if (message_trace_count) {
    message_trace_read = (message_trace_read + 1) % kMessageTraceSize;
    --message_trace_count;
  } else {
    message_trace_dropped = 0;
  }
}
#endif

void fail_output() {
  ++fault_count;
  faulted = true;
  cleanup_pending = true;
  cleanup_key_pending = true;
  queue_read = queue_count = 0;
  ptt_session_active = false;
}

bool enqueue(const uint8_t *bytes, PacketKind kind = PacketKind::Other,
             bool down = false) {
  if (!connected || faulted) {
    return false;
  }
  if (queue_count == kQueueSize) {
    fail_output();
    return false;
  }
  Packet &packet = queue[(queue_read + queue_count) % kQueueSize];
  memcpy(packet.bytes, bytes, 4);
  packet.queued_ms = millis();
  packet.kind = kind;
  packet.down = down;
  ++queue_count;
  return true;
}

bool enqueue_note(uint8_t note, bool down, PacketKind kind) {
  const uint8_t bytes[4] = {
      0x09, static_cast<uint8_t>(0x90 | kChannel), note,
      static_cast<uint8_t>(down ? 127 : 0)};
  return enqueue(bytes, kind, down);
}

bool enqueue_control(uint8_t control, uint8_t value) {
  const uint8_t bytes[4] = {
      0x0B, static_cast<uint8_t>(0xB0 | kChannel), control, value};
  return enqueue(bytes);
}

uint8_t speed_controller_value(uint32_t speed_wpm) {
  const uint32_t speed = speed_wpm < 1 ? 1 :
      speed_wpm > 60 ? 60 : speed_wpm;
  // Thetis decodes round(value / 2.1333 + 1), but ignores value zero.
  const uint32_t value = ((speed - 1) * 21333 + 5000) / 10000;
  return static_cast<uint8_t>(value == 0 ? 1 : value);
}

bool write_packet(const uint8_t *bytes) {
  if (!tud_midi_packet_write(bytes)) {
    return false;
  }
#if defined(DEBUG_MIDI_MESSAGES)
  trace_message(bytes, false);
#endif
  RecentPacket &echo = recent[recent_index];
  memcpy(echo.bytes, bytes, 4);
  echo.sent_us = micros();
  echo.valid = true;
  recent_index = (recent_index + 1) % 32;
  ++sent_packets;
  return true;
}

void flush_output() {
  if (cleanup_pending) {
    if (cleanup_key_pending) {
      const uint8_t key_up[4] = {0x09, 0x99, kKeyNote, 0};
      if (!write_packet(key_up)) {
        return;
      }
      cleanup_key_pending = false;
    }
    if (sent_ptt) {
      const uint8_t ptt_up[4] = {
          0x09, 0x99, kPttNote,
          static_cast<uint8_t>(ptt_mode == Wm8960MidiPttMode::Toggle ? 127 : 0)};
      if (!write_packet(ptt_up)) {
        return;
      }
      sent_ptt = false;
      ptt_session_active = false;
    }
    cleanup_pending = false;
  }
  for (uint16_t count = 0; count < 16 && queue_count; ++count) {
    const Packet &packet = queue[queue_read];
    if (static_cast<uint32_t>(millis() - packet.queued_ms) >=
        kQueueTimeoutMs) {
      fail_output();
      return;
    }
    if (!write_packet(packet.bytes)) {
      return;
    }
    if (packet.kind == PacketKind::Ptt) {
      sent_ptt = packet.down;
    }
    queue_read = (queue_read + 1) % kQueueSize;
    --queue_count;
  }
}

bool is_echo(const uint8_t *bytes, uint32_t window_us = kEchoWindowUs) {
  const uint32_t now = micros();
  for (const RecentPacket &packet : recent) {
    if (packet.valid &&
        static_cast<uint32_t>(now - packet.sent_us) < window_us &&
        memcmp(bytes, packet.bytes, 4) == 0) {
      return true;
    }
  }
  return false;
}

void receive_packet(const uint8_t *bytes) {
  ++received_packets;
#if defined(DEBUG_MIDI_MESSAGES)
  trace_message(bytes, true);
#endif
  const uint8_t cin = bytes[0] & 0x0F;
  const uint8_t status = bytes[1] & 0xF0;
  if ((bytes[0] >> 4) != 0 || cin < 2 ||
      ((cin == 8 || cin == 9 || cin == 11) &&
       (status != (cin << 4) || bytes[2] > 127 || bytes[3] > 127))) {
    ++invalid_packets;
    return;
  }
  // Feedback reports host state and can be byte-identical to what the keyer
  // just sent, so it must be matched before echo suppression.
  const bool feedback_match = WM8960_PTT_FEEDBACK_CHANNEL != 0 &&
      (bytes[1] & 0x0F) + 1 == WM8960_PTT_FEEDBACK_CHANNEL &&
      bytes[2] == WM8960_PTT_FEEDBACK_NUMBER &&
      ((WM8960_PTT_FEEDBACK_TYPE == 11 && cin == 11) ||
       (WM8960_PTT_FEEDBACK_TYPE == 9 && (cin == 8 || cin == 9)));
  if (feedback_match) {
    wm8960_audio_set_external_ptt(cin != 8 && bytes[3] != 0);
    return;
  }
  // POD mode mirrors PC levels and applies PC speed/sidetone; consumed, never echoed back.
  if (control_mode == Wm8960MidiControlMode::Pod && cin == 11 &&
      (bytes[1] & 0x0F) == kChannel) {
    for (uint8_t index = 0; index < sizeof(kHostControlNumbers); ++index) {
      if (bytes[2] == kHostControlNumbers[index]) {
        if (!is_echo(bytes, kOwnFeedbackWindowUs)) {
          host_control_value[index] = bytes[3];
        }
        return;
      }
    }
  }
  // Consumed, never echoed back: the host would answer with more feedback.
  if (WM8960_MIC_LEVEL_FEEDBACK_CHANNEL != 0 && cin == 11 &&
      (bytes[1] & 0x0F) + 1 == WM8960_MIC_LEVEL_FEEDBACK_CHANNEL &&
      bytes[2] == WM8960_MIC_LEVEL_FEEDBACK_CC) {
    if (!is_echo(bytes, kOwnFeedbackWindowUs)) {
      host_mic_value = bytes[3];
    }
    return;
  }
  if (is_echo(bytes)) {
    ++suppressed_echoes;
    return;
  }
  const uint16_t channel = static_cast<uint16_t>(1u << (bytes[1] & 0x0F));
  if (cin == 8 || cin == 9) {
    const bool down = cin == 9 && bytes[3] != 0;
    uint16_t *state = bytes[2] == kKeyNote ? &input_keys :
        bytes[2] == kPttNote ? &input_ptt : nullptr;
    if (state) {
      if (down) {
        *state |= channel;
      } else {
        *state &= static_cast<uint16_t>(~channel);
      }
    }
  } else if (cin == 11 && (bytes[2] == 120 || bytes[2] == 123)) {
    input_keys &= static_cast<uint16_t>(~channel);
    input_ptt &= static_cast<uint16_t>(~channel);
  }
  enqueue(bytes);
}

void reset_session() {
  wm8960_audio_set_external_ptt(false);
  queue_read = queue_count = 0;
  memset(recent, 0, sizeof(recent));
  recent_index = 0;
  input_keys = input_ptt = 0;
  sent_ptt = false;
  ptt_session_active = false;
  faulted = cleanup_pending = false;
  cleanup_key_pending = false;
  for (uint16_t &input : last_control_inputs) {
    input = UINT16_MAX;
  }
  speed_reported = false;
  host_mic_value = -1;
  for (int16_t &value : host_control_value) {
    value = -1;
  }
  wm8960_audio_set_midi_input(false, false, 0);
}
}  // namespace

// Callbacks run in the core USB task; the foreground owns all MIDI queues.
extern "C" void tud_mount_cb() {
  wm8960_audio_set_external_ptt(false);
  ++usb_epoch;
  wm8960_audio_set_midi_input(false, false, 0);
}

extern "C" void tud_umount_cb() {
  wm8960_audio_set_external_ptt(false);
  ++usb_epoch;
  wm8960_audio_set_midi_input(false, false, 0);
}

extern "C" void tud_suspend_cb(bool remote_wakeup_en) {
  wm8960_audio_set_external_ptt(false);
  (void)remote_wakeup_en;
  ++usb_epoch;
  wm8960_audio_set_midi_input(false, false, 0);
}

void wm8960_midi_initialize() {
  initialized = true;
}

void wm8960_midi_service(uint16_t sidetone_frequency, uint32_t speed_wpm,
                         uint8_t master_volume, uint8_t sidetone_volume,
                         uint8_t microphone_volume) {
  if (!initialized || get_core_num() != 0) {
    return;
  }
  {
    CoreMutex usb_lock(&USB.mutex, false);
    if (!usb_lock) {
      return;
    }
    const bool mounted = tud_mounted() && !tud_suspended() && tud_midi_mounted();
    if (serviced_epoch != usb_epoch || mounted != connected) {
      reset_session();
      serviced_epoch = usb_epoch;
      connected = mounted;
      if (connected) {
        enqueue_note(kKeyNote, local_key, PacketKind::Key);
        // Do not replay a PTT toggle or a stale PTT hold on reconnect.
      }
    }
    if (connected) {
      flush_output();
      uint8_t bytes[4];
      for (uint8_t count = 0; count < 16 && tud_midi_packet_read(bytes); ++count) {
        receive_packet(bytes);
      }
      if (control_mode != Wm8960MidiControlMode::Local &&
          (!speed_reported || last_speed_wpm != speed_wpm ||
           static_cast<uint32_t>(millis() - last_speed_report_ms) >=
               kSpeedReportIntervalMs)) {
        const uint8_t value = speed_controller_value(speed_wpm);
        if (enqueue_control(21, value)) {
          last_speed_wpm = speed_wpm;
          last_speed_report_ms = millis();
          speed_reported = true;
        }
      }
      const uint8_t controls[4] = {7, 22, 23, 3};
      const uint16_t inputs[4] = {
          static_cast<uint16_t>(constrain(master_volume, 0, 100)),
          static_cast<uint16_t>(constrain(sidetone_volume, 0, 100)),
          static_cast<uint16_t>(constrain(microphone_volume, 0, 100)),
          static_cast<uint16_t>(constrain(sidetone_frequency, 300, 1000))};
      for (uint8_t index = 0;
           control_mode != Wm8960MidiControlMode::Local && index < 4; ++index) {
        if (last_control_inputs[index] != inputs[index]) {
          const uint8_t value = index == 3
              ? ((inputs[index] - 300u) * 127u + 350u) / 700u
              : (inputs[index] * 127u + 50u) / 100u;
          if (enqueue_control(controls[index], value)) {
            last_control_inputs[index] = inputs[index];
          }
        }
      }
      wm8960_audio_set_midi_input(input_keys != 0, input_ptt != 0,
                                  sidetone_frequency);
      flush_output();
    }
  }
  // Never print from key edges or USB callbacks, or while keying.
  HardwareSerial &port = wm8960_audio_debug_port();
#if defined(DEBUG_MIDI)
  static bool terminal_connected = false;
  const bool terminal_now = static_cast<bool>(port);
  if (terminal_now && !terminal_connected &&
      !local_key && !local_ptt && !input_keys && !input_ptt) {
    wm8960_midi_print_status(port);
    terminal_connected = true;
  } else if (!terminal_now) {
    terminal_connected = false;
  }
#endif
  if (!local_key && !local_ptt && !input_keys && !input_ptt && port) {
#if defined(DEBUG_MIDI_MESSAGES)
    print_message_trace(port);
#endif
    if (reported_fault_count != fault_count) {
      port.println("[MIDI] output queue overflow/timeout; key/PTT release pending, reconnect USB to resume MIDI output");
      reported_fault_count = fault_count;
    }
    if (reported_invalid_packets != invalid_packets) {
      port.printf("[MIDI] rejected invalid/unsupported packets: %lu\r\n",
                  static_cast<unsigned long>(invalid_packets));
      reported_invalid_packets = invalid_packets;
    }
  }
}

void wm8960_midi_key(bool down) {
  if (local_key != down) {
    local_key = down;
    enqueue_note(kKeyNote, down, PacketKind::Key);
  }
}

void wm8960_midi_ptt(bool down) {
  if (local_ptt == down) {
    return;
  }
  local_ptt = down;
  if (ptt_mode != Wm8960MidiPttMode::Off) {
    // A reconnect during an existing hold must not create an unmatched toggle.
    if (!down && !ptt_session_active) {
      return;
    }
    const bool note_on = down || ptt_mode == Wm8960MidiPttMode::Toggle;
    if (enqueue_note(kPttNote, note_on, PacketKind::Ptt)) {
      queue[(queue_read + queue_count - 1) % kQueueSize].down = down;
      ptt_session_active = down;
    }
  }
}

void wm8960_midi_ptt_from_host(bool down) {
  local_ptt = down;
  // The host already holds this state; a later local release must still reach it.
  ptt_session_active = down && ptt_mode != Wm8960MidiPttMode::Off;
  sent_ptt = ptt_session_active;
}

bool wm8960_midi_take_host_mic_level(uint8_t *percent, uint8_t *value) {
  if (host_mic_value < 0) {
    return false;
  }
  *value = static_cast<uint8_t>(host_mic_value);
  *percent = static_cast<uint8_t>((host_mic_value * 100 + 63) / 127);
  host_mic_value = -1;
  // The host already has this level, so do not report it back.
  last_control_inputs[2] = *percent;
  return true;
}

bool wm8960_midi_take_host_control(Wm8960MidiHostControl control, uint16_t *value) {
  if (control >= Wm8960MidiHostControl::Count) {
    return false;
  }
  int16_t &raw = host_control_value[static_cast<uint8_t>(control)];
  if (raw < 0) {
    return false;
  }
  const uint16_t midi = static_cast<uint16_t>(raw);
  raw = -1;
  // The PC already has these values, so do not report them back.
  switch (control) {
    case Wm8960MidiHostControl::Speed:
      if (midi == 0) {
        return false;  // Thetis ignores zero too.
      }
      // Inverse of speed_controller_value: round(value / 2.1333 + 1).
      *value = static_cast<uint16_t>(constrain((midi * 10000u + 10666u) / 21333u + 1u, 1u, 60u));
      last_speed_wpm = *value;
      last_speed_report_ms = millis();
      speed_reported = true;
      return true;
    case Wm8960MidiHostControl::SidetoneFrequency:
      *value = static_cast<uint16_t>(300u + (midi * 700u + 63u) / 127u);
      last_control_inputs[3] = *value;
      return true;
    default:
      *value = static_cast<uint16_t>((midi * 100u + 63u) / 127u);
      last_control_inputs[control == Wm8960MidiHostControl::MasterVolume ? 0 :
                          control == Wm8960MidiHostControl::MicLevel ? 2 : 1] = *value;
      return true;
  }
}

bool wm8960_midi_set_ptt_mode(uint8_t mode) {
  if (mode > static_cast<uint8_t>(Wm8960MidiPttMode::Toggle) ||
      local_ptt || sent_ptt || ptt_session_active || queue_count || cleanup_pending) {
    return false;
  }
  ptt_mode = static_cast<Wm8960MidiPttMode>(mode);
  return true;
}

const char *wm8960_midi_ptt_mode_name(uint8_t mode) {
  switch (mode) {
    case 0: return "OFF";
    case 1: return "ONOFF";
    case 2: return "TOGGLE";
    default: return "INVALID";
  }
}

bool wm8960_midi_set_control_mode(uint8_t mode) {
  if (mode > static_cast<uint8_t>(Wm8960MidiControlMode::Pod)) {
    return false;
  }
  const Wm8960MidiControlMode next = static_cast<Wm8960MidiControlMode>(mode);
  if (next != control_mode) {
    // Report the current values to the PC once control reports are enabled again.
    for (uint16_t &input : last_control_inputs) {
      input = UINT16_MAX;
    }
    speed_reported = false;
    for (int16_t &value : host_control_value) {
      value = -1;
    }
    control_mode = next;
  }
  return true;
}

Wm8960MidiControlMode wm8960_midi_control_mode() {
  return control_mode;
}

const char *wm8960_midi_control_mode_name(uint8_t mode) {
  switch (mode) {
    case 0: return "LOCAL";
    case 1: return "REPORT";
    case 2: return "POD";
    default: return "INVALID";
  }
}

bool wm8960_midi_wheel_step(int direction) {
  if (direction == 0) {
    return true;
  }
  return enqueue_control(20, direction > 0 ? 127 : 1);
}

void wm8960_midi_print_status(HardwareSerial &port) {
  port.printf("MIDI PTT %s; MODE %s; USB %s; output %s; queued %u\r\n",
              wm8960_midi_ptt_mode_name(static_cast<uint8_t>(ptt_mode)),
              wm8960_midi_control_mode_name(static_cast<uint8_t>(control_mode)),
              connected ? "CONNECTED" : "DISCONNECTED",
              faulted ? "FAULTED" : "READY", queue_count);
  port.printf("MIDI sent %lu received %lu echoes %lu invalid %lu faults %lu\r\n",
              static_cast<unsigned long>(sent_packets),
              static_cast<unsigned long>(received_packets),
              static_cast<unsigned long>(suppressed_echoes),
              static_cast<unsigned long>(invalid_packets),
              static_cast<unsigned long>(fault_count));
}
#endif
#endif
