#include "Phaser.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Phaser::Phaser(int sampleRate) : sampleRate_(static_cast<float>(sampleRate)) {
  reset();
}

void Phaser::reset() {
  for (int i = 0; i < 6; ++i) {
    stages_[i][0] = AllpassStage();
    stages_[i][1] = AllpassStage();
  }
  lastOutput_[0] = 0.0f;
  lastOutput_[1] = 0.0f;
}

void Phaser::setParameters(float x, float y) {
  // X -> Rate (0.1Hz to 5Hz)
  targetRate_ = 0.1f + (4.9f * x);

  // Y -> Depth/Feedback (Mix of both for one-knob feel)
  targetDepth_ = y;
}

void Phaser::process(float *buffer, int frames, int channels) {
  const float smooth = 0.002f;

  for (int i = 0; i < frames; ++i) {
    lfoRate_ += (targetRate_ - lfoRate_) * smooth;
    depth_ += (targetDepth_ - depth_) * smooth;
    feedback_ = depth_ * 0.7f; // Linked feedback

    // LFO
    lfoPhase_ += lfoRate_ / sampleRate_;
    if (lfoPhase_ >= 1.0f)
      lfoPhase_ -= 1.0f;

    float lfo = 0.5f + 0.5f * std::sin(2.0f * M_PI * lfoPhase_);

    // Sweep center frequency
    // 200Hz to 2000Hz?
    // Map LFO 0-1 to coefficient -0.9 to 0.9 roughly?
    // Simple mapping:
    float centerFreq = 200.0f + (1800.0f * lfo);
    // Allpass coeff formula roughly: (tan - 1) / (tan + 1)
    // Approx:
    float coeff = (centerFreq / sampleRate_) * 2.0f - 1.0f;
    // Actually that's linear. Let's strictly use 0.0-0.9 range for coefficient
    // directly for "wah" sound
    float allpassCoeff = -0.9f + (1.8f * lfo * depth_);
    // Force valid range
    allpassCoeff = std::max(-0.98f, std::min(allpassCoeff, 0.98f));

    for (int c = 0; c < channels; ++c) {
      if (c >= 2)
        break;

      float input = buffer[i * channels + c];
      float signal = input + (lastOutput_[c] * feedback_);

      for (int s = 0; s < 6; ++s) {
        signal = stages_[s][c].process(signal, allpassCoeff);
      }

      lastOutput_[c] = signal;

      // Mix Wet/Dry
      buffer[i * channels + c] = input + (signal * depth_);
    }
  }
}
