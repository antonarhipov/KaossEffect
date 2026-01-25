#include "Reverb.h"
#include <algorithm>
#include <cmath>

Reverb::Reverb(int sampleRate) : sampleRate_(static_cast<float>(sampleRate)) {
  // Prime number sizes for delay lines to simulate room modes
  // Roughly 30ms to 45ms
  int sizes[] = {static_cast<int>(sampleRate * 0.0297),
                 static_cast<int>(sampleRate * 0.0371),
                 static_cast<int>(sampleRate * 0.0411),
                 static_cast<int>(sampleRate * 0.0437)};

  for (int i = 0; i < 4; ++i) {
    delays_[i].resize(sizes[i]);
  }
  reset();
}

void Reverb::reset() {
  for (int i = 0; i < 4; ++i) {
    std::fill(delays_[i].buffer.begin(), delays_[i].buffer.end(), 0.0f);
    delays_[i].pos = 0;
  }
}

void Reverb::setParameters(float x, float y) {
  // X -> Room Size / Decay Time (Feedback)
  // 0.0 -> small (0.7), 1.0 -> huge (0.95)
  targetFeedback_ = 0.7f + (0.28f * x);

  // Y -> Mix
  targetMix_ = y * 0.6f; // Max 60% wet to avoid gain explosion
}

void Reverb::process(float *buffer, int frames, int channels) {
  const float smooth = 0.002f;

  for (int i = 0; i < frames; ++i) {
    // Smooth
    feedback_ += (targetFeedback_ - feedback_) * smooth;
    mix_ += (targetMix_ - mix_) * smooth;

    // Sum mono input
    float input = 0.0f;
    for (int c = 0; c < channels; ++c)
      input += buffer[i * channels + c];
    input /= (float)channels;

    // Process parallel comb filters
    float wet = 0.0f;
    for (int d = 0; d < 4; ++d) {
      float delayed = delays_[d].buffer[delays_[d].pos];

      // Output of comb is part of wet sig
      wet += delayed;

      // Update delay line with feedback
      float nextVal = input + (delayed * feedback_);
      delays_[d].buffer[delays_[d].pos] = nextVal;

      delays_[d].pos++;
      if (delays_[d].pos >= delays_[d].buffer.size())
        delays_[d].pos = 0;
    }

    // Simple Allpass diffusion could go here, but this is a basic "metallic"
    // reverb which fits Kaoss vibe.

    // Normalize wet sum slightly
    wet *= 0.25f;

    for (int c = 0; c < channels; ++c) {
      buffer[i * channels + c] = buffer[i * channels + c] + wet * mix_;
    }
  }
}
