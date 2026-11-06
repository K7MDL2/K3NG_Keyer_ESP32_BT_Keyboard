#ifndef CW_RX_DECODER_H
#define CW_RX_DECODER_H

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

// USB-mutex-serialized producer, single foreground consumer. No hardware or UI calls.
class CwRxDecoder {
 public:
  static constexpr unsigned InputRate = 48000;
  static constexpr unsigned Rate = 8000;
  static constexpr unsigned Window = 160;
  static constexpr unsigned Hop = 40;
  static constexpr unsigned Capacity = 4096;

  bool configure(unsigned threshold, unsigned frequency, unsigned wpm = 0) {
    if (threshold > 100 || frequency < 300 || frequency > 1200 ||
        (wpm != 0 && (wpm < 5 || wpm > 60))) return false;
    threshold_ = threshold;
    sensitivity_ = std::pow(1.6f, (static_cast<float>(threshold) - 50) / 50);
    frequency_ = frequency;
    fixed_wpm_ = wpm;
    enabled_.store(false);
    reset_stream();
    enabled_.store(threshold != 0);
    return true;
  }

  void submit(const int16_t *stereo, size_t frames, unsigned channel) {
    if (!enabled_.load()) return;
    for (size_t frame = 0; frame < frames; ++frame) {
      sum_ += stereo[frame * 2 + channel];
      if (++decimation_ != 6) continue;
      const int16_t sample = static_cast<int16_t>(sum_ / 6);
      sum_ = 0;
      decimation_ = 0;
      const uint32_t write = write_.load(std::memory_order_relaxed);
      if (write - read_.load(std::memory_order_acquire) == Capacity) {
        dropped_.fetch_add(1);
        discontinuity_.store(true);
        continue;
      }
      samples_[write & (Capacity - 1)] = sample;
      write_.store(write + 1, std::memory_order_release);
    }
  }

  // Call under the producer's USB mutex on configuration/USB lifecycle changes.
  void reset_stream() {
    decimation_ = 0;
    sum_ = 0;
    discontinuity_.store(true);
  }

  template<class Emit>
  void process(unsigned budget, Emit emit, uint32_t (*clock_us)() = nullptr,
               uint32_t time_budget_us = 0) {
    const uint32_t started_us = clock_us ? clock_us() : 0;
    if (discontinuity_.exchange(false)) {
      read_.store(write_.load());
      reset_dsp();
    }
    if (!enabled_.load()) return;
    while (budget--) {
      if (discontinuity_.exchange(false)) {
        read_.store(write_.load());
        reset_dsp();
      }
      const uint32_t read = read_.load(std::memory_order_relaxed);
      if (read == write_.load(std::memory_order_acquire)) break;
      window_[count_++] = samples_[read & (Capacity - 1)];
      read_.store(read + 1, std::memory_order_release);
      if (count_ != Window) continue;
      analyze(emit);
      std::memmove(window_, window_ + Hop, (Window - Hop) * sizeof(window_[0]));
      count_ = Window - Hop;
      if (clock_us && static_cast<uint32_t>(clock_us() - started_us) >= time_budget_us) break;
    }
  }

  unsigned threshold() const { return threshold_; }
  unsigned frequency() const { return frequency_; }
  unsigned tracked_frequency() const { return tracked_frequency_; }
  uint32_t frequency_switches() const { return tracking_switches_; }
  unsigned fixed_wpm() const { return fixed_wpm_; }
  unsigned wpm() const { return static_cast<unsigned>(1200.0f / dot_ + 0.5f); }
  float signal() const { return signal_; }
  float noise() const { return noise_; }
  bool tone() const { return tone_; }
  uint32_t dropped() const { return dropped_.load(); }
  uint32_t pending_samples() const {
    return write_.load(std::memory_order_acquire) - read_.load(std::memory_order_acquire);
  }

 private:
  void track_frequency(float energy) {
    float best = 0;
    unsigned best_frequency = frequency_;
    for (unsigned i = 0; i < tracking_count_; ++i) {
      const float coefficient = tracking_coefficients_[i];
      float q1 = 0, q2 = 0;
      for (int16_t sample : window_) {
        const float q0 = coefficient * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
      }
      const float power = std::max(0.0f, q1*q1 + q2*q2 - coefficient*q1*q2);
      if (power > best) {
        best = power;
        best_frequency = tracking_start_ + i * 5;
      }
    }
    const float amplitude = 2.0f * std::sqrt(best) / Window;
    const bool credible = amplitude >= 1.15f * std::sqrt(energy / Window) &&
        amplitude >= std::max(1000.0f, noise_ * 6.0f);
    const unsigned difference = best_frequency > tracked_frequency_ ?
        best_frequency - tracked_frequency_ : tracked_frequency_ - best_frequency;
    const bool matching = credible && difference < 20;
    if (matching) tracking_absent_ = 0;
    else if (tracking_absent_ < 150) tracking_absent_ += 5;
    if (tracking_absent_ >= 150) tracking_locked_ = false;
    if (!credible || (tracking_locked_ && !matching)) {
      tracking_support_ = 0;
      return;
    }
    const unsigned candidate_difference = best_frequency > tracking_candidate_ ?
        best_frequency - tracking_candidate_ : tracking_candidate_ - best_frequency;
    if (candidate_difference > 5) {
      tracking_candidate_ = best_frequency;
      tracking_support_ = 1;
    } else if (tracking_support_ < 4) ++tracking_support_;
    if (tracking_support_ >= 4) {
      if (difference >= 20) {
        tracked_frequency_ = best_frequency;
        coefficient_ = tracking_coefficients_[(best_frequency - tracking_start_) / 5];
        ++tracking_switches_;
      }
      tracking_locked_ = true;
      tracking_absent_ = 0;
    }
  }

  void reset_dsp() {
    tracked_frequency_ = tracking_candidate_ = frequency_;
    tracking_support_ = tracking_absent_ = tracking_switches_ = 0;
    tracking_locked_ = false;
    tracking_start_ = frequency_ - std::min(40u, (frequency_ - 300) / 5 * 5);
    const unsigned end = frequency_ + std::min(40u, (1200 - frequency_) / 5 * 5);
    tracking_count_ = (end - tracking_start_) / 5 + 1;
    for (unsigned i = 0; i < tracking_count_; ++i)
      tracking_coefficients_[i] =
          2.0f * std::cos(6.28318530718f * (tracking_start_ + i * 5) / Rate);
    count_ = 0;
    tone_ = candidate_ = false;
    candidate_count_ = quality_hops_ = quality_run_ = quality_streak_ = 0;
    duration_ = 0;
    pattern_ = 0;
    isolated_weak_mark_ = false;
    marks_ = 0;
    word_sent_ = true;
    last_mark_ = last_gap_ = preceding_gap_ = 0;
    dot_ = fixed_wpm_ ? 1200.0f / fixed_wpm_ : 60.0f;
    noise_ = 12.0f;
    noise_count_ = noise_write_ = 0;
    signal_ = 0;
    peak_ = 0;
    coefficient_ = 2.0f * std::cos(6.28318530718f * frequency_ / Rate);
  }

  template<class Emit>
  void analyze(Emit emit) {
    float q1 = 0, q2 = 0, energy = 0;
    for (int16_t value : window_) energy += static_cast<float>(value) * value;
    track_frequency(energy);
    for (int16_t value : window_) {
      const float q0 = coefficient_ * q1 - q2 + value;
      q2 = q1;
      q1 = q0;
    }
    signal_ = 2.0f * std::sqrt(std::max(0.0f, q1*q1 + q2*q2 - coefficient_*q1*q2)) / Window;
    // 1..100 selects an amplitude floor of approximately -66..-26 dBFS.
    const float floor = 16.0f * std::pow(100.0f, (threshold_ - 1) / 99.0f);
    const float residual = std::sqrt(std::max(0.0f, energy / Window - signal_ * signal_ * 0.5f));
    noise_history_[noise_write_] = std::min(signal_, residual * 0.6f);
    noise_write_ = (noise_write_ + 1) % 128;
    if (noise_count_ < 128) ++noise_count_;
    if (noise_count_ >= 16 && noise_write_ % 16 == 0) {
      float sorted[128];
      std::copy(noise_history_, noise_history_ + noise_count_, sorted);
      std::sort(sorted, sorted + noise_count_);
      // Learn the lower envelope even if background audio initially holds tone on.
      noise_ = std::max(4.0f, sorted[noise_count_ / 5]);
    }
    peak_ = std::max(signal_, peak_ * 0.98f);
    const float noise_limit = noise_ * (tone_ ? 2.2f : 3.0f) * sensitivity_;
    const float peak_limit = peak_ * (tone_ ? 0.35f : 0.5f * sensitivity_);
    const float limit = std::max(floor, std::max(noise_limit, peak_limit));
    const bool detected = signal_ > limit &&
        signal_ > (tone_ ? 0.65f : 1.0f) * std::sqrt(energy / Window);
    if (candidate_ != detected) {
      candidate_ = detected;
      candidate_count_ = 1;
      if (detected && !tone_) quality_hops_ = quality_run_ = quality_streak_ = 0;
    } else if (candidate_count_ < 3) ++candidate_count_;
    if (detected && signal_ > 1.15f * std::sqrt(energy / Window) &&
        signal_ > 8.0f * noise_) {
      if (quality_hops_ < 3) ++quality_hops_;
    }
    const float isolated_coherence = dot_ < 30.0f ? 1.15f : 1.25f;
    if (detected && signal_ > isolated_coherence * std::sqrt(energy / Window) &&
        signal_ > 8.0f * noise_) {
      if (quality_run_ < 3) ++quality_run_;
      quality_streak_ = std::max(quality_streak_, quality_run_);
    } else quality_run_ = 0;
    duration_ += 5;
    const bool timing_known = fixed_wpm_ || marks_ >= 4;
    const unsigned confirmation = detected || !timing_known ? 2u :
        std::max(2u, std::min(3u, static_cast<unsigned>(std::ceil(dot_ * 0.25f / 5))));
    if (candidate_count_ >= confirmation && candidate_ != tone_) {
      // Confirm transitions later, but retain the first candidate's sample time.
      const unsigned delay = (candidate_count_ - 1) * 5;
      const unsigned duration = duration_ - delay;
      duration_ = delay;
      tone_ = candidate_;
      if (tone_) preceding_gap_ = duration;
      if (!tone_) {
        const float minimum_mark = timing_known ? std::max(15.0f, dot_ * 0.5f) : 15.0f;
        const bool rescued = duration >= 15 && duration < minimum_mark &&
            quality_hops_ >= 2 && pattern_ && last_mark_ >= 2.0f * dot_ &&
            preceding_gap_ >= 0.5f * dot_ &&
            preceding_gap_ <= 1.75f * dot_;
        const bool isolated = timing_known && !pattern_ &&
            preceding_gap_ >= std::max(200.0f, 5.0f * dot_);
        if ((duration >= minimum_mark || rescued) && duration <= 900) {
          isolated_weak_mark_ = isolated && quality_streak_ < 2;
          // A clipped dit is usable text evidence, not a new speed measurement.
          if (!fixed_wpm_ && !rescued && !isolated_weak_mark_) {
            if (pattern_ && preceding_gap_ >= 15 && preceding_gap_ < 2 * dot_)
              last_gap_ = preceding_gap_;
            // Pair a dit with its gap to cancel opposite detector edge delays.
            const unsigned timing = last_gap_ && duration < 2 * dot_ &&
                last_gap_ >= duration * 0.5f && last_gap_ <= duration * 2.0f ?
                (duration + last_gap_) / 2 : duration;
            history_[marks_ % 20] = static_cast<uint16_t>(timing);
            ++marks_;
            const unsigned size = std::min(marks_, 20u);
            if (size >= 4) {
              uint16_t sorted[20];
              std::copy(history_, history_ + size, sorted);
              std::sort(sorted, sorted + size);
              const float candidate = sorted[size / 4];
              unsigned support = 0;
              for (unsigned i = 0; i < size; ++i)
                if (sorted[i] >= candidate * 0.8f && sorted[i] <= candidate * 1.2f) ++support;
              if (support >= 3)
                dot_ = std::max(20.0f, std::min(240.0f, dot_ * 0.6f + candidate * 0.4f));
            }
          }
          if (pattern_ < 10000000)
            pattern_ = pattern_ * 10 + (duration < 2 * dot_ ? 1 : 2);
          else pattern_ = 0;
          last_mark_ = duration;
          if (!isolated_weak_mark_) word_sent_ = false;
        } else {
          // Rejected pulses are still part of the original key-up gap.
          duration_ += preceding_gap_ + duration;
        }
      }
    }
    if (!tone_ && !candidate_ && pattern_ && duration_ >= 2.0f * dot_) {
      // A following element supports a weak onset; a lone noise mark does not.
      if (!isolated_weak_mark_) emit(pattern_);
      pattern_ = 0;
      isolated_weak_mark_ = false;
    }
    if (!tone_ && !candidate_ && !pattern_ && !word_sent_ && duration_ >= 5.0f * dot_) {
      emit(0); // Word space; caller uses its existing Morse character table.
      word_sent_ = true;
    }
  }

  std::atomic<bool> enabled_{false}, discontinuity_{false};
  std::atomic<uint32_t> write_{0}, read_{0}, dropped_{0};
  int16_t samples_[Capacity]{}, window_[Window]{};
  uint16_t history_[20]{};
  float noise_history_[128]{};
  int32_t sum_ = 0;
  unsigned decimation_ = 0, count_ = 0, threshold_ = 0, frequency_ = 600, fixed_wpm_ = 0;
  unsigned candidate_count_ = 0, quality_hops_ = 0, duration_ = 0, marks_ = 0;
  unsigned quality_run_ = 0, quality_streak_ = 0;
  unsigned last_mark_ = 0, last_gap_ = 0, preceding_gap_ = 0;
  unsigned noise_count_ = 0, noise_write_ = 0;
  long pattern_ = 0;
  bool tone_ = false, candidate_ = false, word_sent_ = true;
  bool isolated_weak_mark_ = false;
  float coefficient_ = 0, noise_ = 12, signal_ = 0, peak_ = 0, dot_ = 60, sensitivity_ = 1;
  float tracking_coefficients_[17]{};
  unsigned tracked_frequency_ = 600, tracking_candidate_ = 600;
  unsigned tracking_start_ = 560, tracking_count_ = 17;
  unsigned tracking_support_ = 0, tracking_absent_ = 0;
  uint32_t tracking_switches_ = 0;
  bool tracking_locked_ = false;
};

#endif
