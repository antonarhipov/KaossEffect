#include "Glitch.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

Glitch::Glitch(int sampleRate) : sampleRate_(static_cast<float>(sampleRate)) {
  // 1 second buffer is plenty
  bufferSize_ = sampleRate;
  buffer_.resize(bufferSize_ * 2, 0.0f); // Stereo
  reset();
}

void Glitch::reset() {
  std::fill(buffer_.begin(), buffer_.end(), 0.0f);
  writePos_ = 0;
  active_ = false;
  grainCounter_ = 0;
}

void Glitch::setParameters(float x, float y) {
  // X -> Grain Size (Speed)
  // 0.0 -> 10ms (fast buzz)
  // 1.0 -> 500ms (slow repeat)
  targetRate_ = 0.01f + (0.49f * x);

  // Y -> Mix / Active Threshold
  // If y > 0.1, we start stuttering?
  // Or just always stutter if selected?
  // Let's use Y as Mix + Pitch shift randomness?
  // For now, simple Mix.
  targetMix_ = y;
}

void Glitch::process(float *buffer, int frames, int channels) {
  const float smooth = 0.1f; // Fast smoothing for glitch

  grainSizeTime_ += (targetRate_ - grainSizeTime_) * smooth;
  mix_ += (targetMix_ - mix_) * smooth;

  grainSizeSamples_ = static_cast<int>(grainSizeTime_ * sampleRate_);
  if (grainSizeSamples_ < 100)
    grainSizeSamples_ = 100;

  for (int i = 0; i < frames; ++i) {
    // Always record to circular buffer
    for (int c = 0; c < channels; ++c) {
      if (c >= 2)
        break;
      buffer_[writePos_ * 2 + c] = buffer[i * channels + c];
    }

    // Stutter Logic
    // If mix is high enough, replace output with read from earlier
    if (mix_ > 0.05f) {
      if (!active_) {
        // Trigger capture
        // "Freeze" the read pointer relative to current write?
        // Actually, Glitch usually repeats the LAST chunk.
        // So readPos should start at writePos - grainSizeSamples
        readPos_ = writePos_ - grainSizeSamples_;
        while (readPos_ < 0)
          readPos_ += bufferSize_;
        active_ = true;
        grainCounter_ = 0;
      }

      // Playback from frozen loop
      for (int c = 0; c < channels; ++c) {
        if (c >= 2)
          break;
        // Linear crossfade from Dry to Wet based on Mix
        float dry = buffer[i * channels + c];
        float wet = buffer_[readPos_ * 2 + c];
        buffer[i * channels + c] = dry * (1.0f - mix_) + wet * mix_;
      }

      readPos_++;
      grainCounter_++;
      if (readPos_ >= bufferSize_)
        readPos_ = 0;

      // Loop the grain
      if (grainCounter_ >= grainSizeSamples_) {
        // Reset read pointer to start of grain
        readPos_ -= grainSizeSamples_;
        while (readPos_ < 0)
          readPos_ += bufferSize_;
        grainCounter_ = 0;
      }

    } else {
      active_ = false;
    }

    writePos_++;
    if (writePos_ >= bufferSize_)
      writePos_ = 0;
  }
}
