#include "RingMod.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RingMod::RingMod(int sampleRate)
    : sampleRate_(static_cast<float>(sampleRate)) {}

void RingMod::reset() { carrierPhase_ = 0.0f; }

void RingMod::setParameters(float x, float y) {
  // X -> Frequency (Log scale 50Hz to 4000Hz)
  // 50 * (80^x)
  targetFreq_ = 50.0f * std::pow(80.0f, x);

  // Y -> Mix
  targetMix_ = y;
}

void RingMod::process(float *buffer, int frames, int channels) {
  const float smooth = 0.002f;

  for (int i = 0; i < frames; ++i) {
    carrierFreq_ += (targetFreq_ - carrierFreq_) * smooth;
    mix_ += (targetMix_ - mix_) * smooth;

    carrierPhase_ += carrierFreq_ / sampleRate_;
    if (carrierPhase_ >= 1.0f)
      carrierPhase_ -= 1.0f;

    float carrier = std::sin(2.0f * M_PI * carrierPhase_);

    for (int c = 0; c < channels; ++c) {
      float input = buffer[i * channels + c];
      float wet = input * carrier;
      buffer[i * channels + c] = input * (1.0f - mix_) + wet * mix_;
    }
  }
}
