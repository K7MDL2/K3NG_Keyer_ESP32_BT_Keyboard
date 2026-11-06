#include "cw_rx_decoder.h"
#include <cassert>
#include <vector>
#include <cstdio>
#include <fstream>
#include <string>
uint32_t budget_clock = 0;
uint32_t test_clock_us() { budget_clock += 500; return budget_clock; }

struct AudioTest {
  CwRxDecoder decoder;
  std::vector<long> result;
  unsigned frequency = 600, amplitude = 3000, channel = 0, index = 0;
  unsigned random = 1;
  unsigned noise_amplitude = 500;

  AudioTest(unsigned threshold, unsigned hz, unsigned wpm) : frequency(hz) {
    assert(decoder.configure(threshold, hz, wpm));
    process();
  }
  void process() { decoder.process(160, [&](long p) { result.push_back(p); }); }
  void feed(bool tone, unsigned ms, bool noise = false) {
    for (unsigned frame = 0; frame < ms * 48; frame += 48) {
      int16_t samples[96];
      for (unsigned n = 0; n < 48; ++n) {
        random = random * 1664525u + 1013904223u;
        const int noise_sample = noise ?
            static_cast<int>((random >> 16) % (2*noise_amplitude+1)) - static_cast<int>(noise_amplitude) : 0;
        samples[n*2] = static_cast<int16_t>(noise_sample +
            (tone ? amplitude * std::sin(6.28318530718 * frequency * index / 48000) : 0));
        samples[n*2+1] = -samples[n*2];
        ++index;
      }
      decoder.submit(samples,48,channel);
      process();
    }
  }
  void letter(unsigned pattern, unsigned dit) {
    std::vector<unsigned> elements;
    do { elements.push_back(pattern % 10); pattern /= 10; } while (pattern);
    for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
      feed(true, *it == 2 ? dit*3 : dit);
      feed(false, it+1 == elements.rend() ? dit*3 : dit);
    }
  }
  void sos(unsigned wpm) {
    const unsigned dit = 1200/wpm;
    feed(false,100);
    for (unsigned pattern : {111u,222u,111u}) letter(pattern,dit);
    feed(false,dit*7);
  }
};

char recording_character(long pattern) {
  if (!pattern) return ' ';
  const char *codes[] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....",
      "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
      "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--..",
      "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...",
      "---..", "----.", "-..-.", "-....-", ".-.-.-", "--..--", "..--.."};
  const char *characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/-.,?";
  for (unsigned i=0; i<sizeof(codes)/sizeof(codes[0]); ++i) {
    long number = 0;
    for (const char *element=codes[i]; *element; ++element)
      number = number*10 + (*element == '.' ? 1 : 2);
    if (number == pattern) return characters[i];
  }
  return '*';
}

int main(int argc, char **argv) {
  if (argc == 3) {
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) { std::fprintf(stderr,"Cannot open recording PCM\n"); return 1; }
    CwRxDecoder recording;
    assert(recording.configure(50,700,20));
    std::string text;
    auto emit = [&](long pattern) { text += recording_character(pattern); };
    recording.process(1280,emit);
    int16_t block[480];
    while (file.read(reinterpret_cast<char*>(block),sizeof(block))) {
      recording.submit(block,240,0);
      recording.process(1280,emit);
    }
    if (!file.eof() || file.gcount() % 4 != 0) {
      std::fprintf(stderr,"Invalid stereo PCM recording\n"); return 1;
    }
    if (file.gcount()) {
      recording.submit(block,static_cast<size_t>(file.gcount())/4,0);
      recording.process(1280,emit);
    }
    std::printf("Recording decode (700 Hz, threshold 50, fixed 20 WPM): %s\n",text.c_str());
    assert(recording.dropped() == 0);
    if (text.find(argv[2]) == std::string::npos) {
      std::fprintf(stderr,"Expected recording substring missing: %s\n",argv[2]); return 1;
    }
    std::puts("PASS: recorded reference substring with zero PCM drops (not a full-transcript accuracy test)");
  } else if (argc != 1) {
    std::fprintf(stderr,"Usage: test [48k-stereo-s16-PCM expected-substring]\n");
    return 1;
  }
  CwRxDecoder bounded;
  assert(bounded.configure(20,600,20));
  bounded.process(1280,[](long) {});
  std::vector<int16_t> backlog(7680*2);
  bounded.submit(backlog.data(),7680,0);
  const auto pending = bounded.pending_samples();
  bounded.process(1280,[](long) {},test_clock_us,2000);
  assert(bounded.pending_samples() > 0 && bounded.pending_samples() < pending);
  assert(budget_clock == 2500); // Four DSP windows, not the full 1280-sample batch.
  // A 50 ms foreground pass produces 400 samples; a 320-sample budget cannot keep up.
  for (unsigned budget : {320u,1280u}) {
    CwRxDecoder queued;
    assert(queued.configure(20,600,20));
    queued.process(budget,[](long) {});
    std::vector<int16_t> block(2400*2);
    for (unsigned pass=0; pass<150; ++pass) {
      queued.submit(block.data(),2400,0);
      queued.process(budget,[](long) {});
    }
    if (budget == 320) assert(queued.dropped() > 0);
    else assert(queued.dropped() == 0 && queued.pending_samples() == 0);
  }
  for (unsigned wpm : {5u,12u,20u,40u,60u}) {
    for (unsigned hz : {300u,600u,750u,1200u}) {
      for (unsigned channel : {0u,1u}) {
        AudioTest test(20,hz,wpm);
        test.channel = channel;
        test.sos(wpm);
        assert((test.result == std::vector<long>{111,222,111,0}));
        assert(test.decoder.dropped() == 0);
      }
    }
    AudioTest automatic(20,600,0);
    automatic.feed(false,100);
    for (unsigned n = 0; n < 8; ++n) automatic.letter(1112,1200/wpm);
    automatic.feed(false,1200/wpm*7);
    automatic.result.clear();
    automatic.sos(wpm);
    assert((automatic.result == std::vector<long>{111,222,111,0}));
    std::printf("auto input %u estimated %u\n", wpm, automatic.decoder.wpm());
    std::fflush(stdout);
    assert(automatic.decoder.wpm() >= wpm-1 && automatic.decoder.wpm() <= wpm+1);
  }
  AudioTest low(1,600,20), strict(100,600,20);
  AudioTest short_burst(76,700,27);
  short_burst.feed(false,1000);
  short_burst.feed(true,15);
  short_burst.feed(false,500);
  assert(short_burst.result.empty());
  AudioTest isolated_noise(76,700,27);
  isolated_noise.feed(false,2000);
  isolated_noise.noise_amplitude = 6000;
  isolated_noise.feed(true,45,true);
  isolated_noise.feed(false,400);
  assert(isolated_noise.result.empty());
  AudioTest weak_onset(76,700,27);
  weak_onset.feed(false,2000);
  weak_onset.noise_amplitude = 6000;
  weak_onset.feed(true,45,true);
  weak_onset.feed(false,45);
  weak_onset.feed(true,45);
  weak_onset.feed(false,45);
  weak_onset.feed(true,135);
  weak_onset.feed(false,400);
  assert((weak_onset.result == std::vector<long>{112,0}));
  for (unsigned wpm = 5; wpm <= 60; ++wpm) {
    for (unsigned hz : {300u,700u,1200u}) {
      AudioTest isolated_tone(76,hz,wpm);
      isolated_tone.feed(false,2000);
      isolated_tone.letter(1,1200/wpm);
      isolated_tone.feed(false,1200/wpm*7);
      isolated_tone.letter(2,1200/wpm);
      isolated_tone.feed(false,1200/wpm*7);
      assert((isolated_tone.result == std::vector<long>{1,0,2,0}));
    }
  }
  AudioTest dropout(76,700,27);
  dropout.feed(false,100);
  dropout.feed(true,45);
  dropout.feed(false,45);
  dropout.feed(true,135);
  dropout.feed(false,45);
  dropout.feed(true,75);
  dropout.feed(false,15);
  dropout.feed(true,45);
  dropout.feed(false,400);
  assert((dropout.result == std::vector<long>{122,0}));
  AudioTest short_dit(76,700,27);
  short_dit.feed(false,100);
  short_dit.feed(true,45);
  short_dit.feed(false,45);
  short_dit.feed(true,135);
  short_dit.feed(false,45);
  short_dit.feed(true,15);
  short_dit.feed(false,400);
  assert((short_dit.result == std::vector<long>{121,0}));
  AudioTest learned_short_dit(76,700,0);
  for (unsigned n = 0; n < 8; ++n) learned_short_dit.letter(1112,45);
  learned_short_dit.feed(false,400);
  learned_short_dit.result.clear();
  learned_short_dit.feed(true,45);
  learned_short_dit.feed(false,45);
  learned_short_dit.feed(true,135);
  learned_short_dit.feed(false,45);
  const unsigned before_rescue = learned_short_dit.decoder.wpm();
  learned_short_dit.feed(true,15);
  learned_short_dit.feed(false,400);
  assert((learned_short_dit.result == std::vector<long>{121,0}));
  assert(learned_short_dit.decoder.wpm() == before_rescue);
  for (unsigned hz : {700u,720u}) {
    AudioTest unreliable_dit(76,700,27);
    unreliable_dit.feed(false,100);
    unreliable_dit.feed(true,45);
    unreliable_dit.feed(false,45);
    unreliable_dit.feed(true,135);
    unreliable_dit.feed(false,45);
    unreliable_dit.frequency = hz;
    unreliable_dit.noise_amplitude = 6000;
    unreliable_dit.feed(true,15,true);
    unreliable_dit.feed(false,400);
    assert((unreliable_dit.result == std::vector<long>{12,0}));
  }
  for (unsigned wpm = 5; wpm <= 60; ++wpm) {
    AudioTest fast_edges(76,700,wpm);
    fast_edges.sos(wpm);
    assert((fast_edges.result == std::vector<long>{111,222,111,0}));
  }
  AudioTest burst_gap(76,700,27);
  burst_gap.feed(true,45);
  burst_gap.feed(false,65);
  burst_gap.feed(true,15);
  burst_gap.feed(false,65);
  burst_gap.feed(true,45);
  burst_gap.feed(false,400);
  assert((burst_gap.result == std::vector<long>{1,1,0}));
  AudioTest learned_burst(76,700,0);
  for (unsigned n = 0; n < 8; ++n) learned_burst.letter(1112,45);
  learned_burst.feed(false,400);
  learned_burst.result.clear();
  const unsigned learned_wpm = learned_burst.decoder.wpm();
  for (unsigned n = 0; n < 24; ++n) {
    learned_burst.feed(true,15);
    learned_burst.feed(false,300);
  }
  assert(learned_burst.result.empty());
  assert(learned_burst.decoder.wpm() == learned_wpm);
  learned_burst.noise_amplitude = 6000;
  learned_burst.feed(true,45,true);
  learned_burst.feed(false,400);
  assert(learned_burst.result.empty());
  assert(learned_burst.decoder.wpm() == learned_wpm);
  AudioTest fading(20,700,20);
  fading.letter(1112,60);
  fading.result.clear();
  fading.amplitude = 900;
  fading.letter(111,60);
  fading.feed(false,420);
  assert((fading.result == std::vector<long>{111,0}));
  AudioTest adaptive_low(1,700,20), adaptive_high(100,700,20);
  for (AudioTest *test : {&adaptive_low, &adaptive_high}) {
    test->amplitude = 12000;
    test->letter(1112,60);
    test->result.clear();
    test->amplitude = 3600;
    test->letter(111,60);
    test->feed(false,420);
    assert(test->decoder.dropped() == 0);
  }
  assert((adaptive_low.result == std::vector<long>{111,0}));
  assert(adaptive_high.result != adaptive_low.result);
  AudioTest short_gap(20,600,0);
  for (unsigned n=0; n<8; ++n) short_gap.letter(1112,60);
  short_gap.feed(false,420);
  assert(short_gap.decoder.wpm() >= 19 && short_gap.decoder.wpm() <= 21);
  short_gap.feed(true,60);
  short_gap.feed(false,20);
  short_gap.feed(true,20);
  assert(short_gap.decoder.wpm() >= 19 && short_gap.decoder.wpm() <= 21);
  for (unsigned threshold : {1u,20u,50u,89u,90u,100u}) {
    AudioTest automatic(threshold,600,0);
    for (unsigned n=0; n<8; ++n) automatic.letter(1112,1200/28);
    automatic.feed(false,1200/28*7);
    automatic.result.clear();
    automatic.sos(28);
    assert((automatic.result == std::vector<long>{111,222,111,0}));
    std::printf("auto threshold %u estimated %u\n", threshold, automatic.decoder.wpm());
    std::fflush(stdout);
    assert(automatic.decoder.wpm() >= 27 && automatic.decoder.wpm() <= 29);
  }
  low.amplitude = strict.amplitude = 300;
  low.sos(20);
  strict.sos(20);
  assert((low.result == std::vector<long>{111,222,111,0}));
  assert(strict.result.empty());
  AudioTest noise(20,600,20);
  noise.feed(false,5000,true);
  assert(noise.result.empty());
  AudioTest burst_noise(50,700,20);
  burst_noise.feed(false,1000);
  burst_noise.noise_amplitude = 6000;
  burst_noise.feed(false,100,true);
  burst_noise.feed(false,500);
  assert(burst_noise.result.empty());
  AudioTest nearby_tone(50,700,20);
  nearby_tone.frequency = 730;
  nearby_tone.sos(20);
  assert((nearby_tone.result == std::vector<long>{111,222,111,0}));
  assert(nearby_tone.decoder.frequency() == 700);
  assert(nearby_tone.decoder.tracked_frequency() >= 720 &&
      nearby_tone.decoder.tracked_frequency() <= 740);
  AudioTest tracking(76,700,27);
  tracking.sos(27);
  tracking.frequency = 730;
  tracking.sos(27);
  tracking.frequency = 700;
  tracking.sos(27);
  assert((tracking.result == std::vector<long>{111,222,111,0,111,222,111,0,111,222,111,0}));
  assert(tracking.decoder.frequency_switches() == 2);
  assert(tracking.decoder.tracked_frequency() == 700);
  AudioTest interference(76,700,27);
  interference.feed(true,200);
  interference.feed(false,45);
  interference.frequency = 730;
  interference.feed(true,50);
  interference.frequency = 700;
  interference.feed(true,200);
  assert(interference.decoder.frequency_switches() == 0);
  for (unsigned center : {300u,319u,700u,1181u,1200u}) {
    for (int offset : {-30,30}) {
      const int carrier = static_cast<int>(center) + offset;
      if (carrier < 300 || carrier > 1200) continue;
      AudioTest edge_tracking(76,center,27);
      edge_tracking.frequency = static_cast<unsigned>(carrier);
      edge_tracking.sos(27);
      assert((edge_tracking.result == std::vector<long>{111,222,111,0}));
      assert(edge_tracking.decoder.frequency() == center);
      const unsigned tracked = edge_tracking.decoder.tracked_frequency();
      assert(tracked >= 300 && tracked <= 1200);
      assert(std::abs(static_cast<int>(tracked) - carrier) <= 5);
      assert(edge_tracking.decoder.dropped() == 0);
    }
  }
  interference.feed(false,500);
  interference.frequency = 730;
  interference.feed(true,100);
  assert(interference.decoder.frequency_switches() == 1);
  assert(interference.decoder.configure(76,900,27));
  interference.process();
  assert(interference.decoder.frequency() == 900);
  assert(interference.decoder.tracked_frequency() == 900);
  assert(interference.decoder.frequency_switches() == 0);
  // Starting on a loud background must not freeze the noise estimator at 12.
  AudioTest loud_noise(20,700,0);
  loud_noise.noise_amplitude = 6000;
  loud_noise.feed(false,2000,true);
  assert(loud_noise.decoder.noise() > 100);
  assert(!loud_noise.decoder.tone());
  for (unsigned hz : {600u,750u}) {
    AudioTest carrier(20,hz,20);
    carrier.feed(true,2000);
    assert(carrier.decoder.tone());
  }
  AudioTest off_frequency(20,600,20);
  off_frequency.frequency = 1000;
  off_frequency.sos(20);
  assert(off_frequency.result.empty());
  assert(off_frequency.decoder.frequency_switches() == 0);
  AudioTest stopped(20,600,20);
  stopped.feed(true,60);
  stopped.feed(false,60);
  assert(stopped.decoder.configure(0,600,20));
  stopped.feed(false,1000);
  assert(stopped.result.empty());
  assert(stopped.decoder.configure(20,750,20));
  stopped.frequency = 750;
  stopped.process();
  stopped.sos(20);
  assert((stopped.result == std::vector<long>{111,222,111,0}));
  AudioTest overflow(20,600,20);
  int16_t packet[96]{};
  for (unsigned i=0; i<1000; ++i) overflow.decoder.submit(packet,48,0);
  assert(overflow.decoder.dropped() > 0);
  overflow.process();
  assert(overflow.result.empty());
  overflow.sos(20);
  assert((overflow.result == std::vector<long>{111,222,111,0}));
  CwRxDecoder decoder;
  assert(!decoder.configure(101,600));
  assert(!decoder.configure(1,299));
  assert(!decoder.configure(1,600,61));
  std::puts("PASS: CW PCM fixed/learned 5..60 WPM; 300..1200 Hz; both stereo channels; stricter threshold; noise; off; reset/recovery");
}
