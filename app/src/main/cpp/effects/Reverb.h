#ifndef KAOSSEFFECT_REVERB_H
#define KAOSSEFFECT_REVERB_H

#include <vector>

class Reverb {
public:
  Reverb(int sampleRate);
  void process(float *buffer, int frames, int channels);
  void setParameters(float x, float y);
  void reset();

private:
  float sampleRate_;

  // Simple Schroeder Recirculating Delay (Comb-like)
  struct DelayLine {
    std::vector<float> buffer;
    int pos = 0;
    void resize(int size) { buffer.resize(size, 0.0f); }
  };

  // multiple delay lines for density
  DelayLine delays_[4];

  // Parameters
  float feedback_ = 0.8f;
  float mix_ = 0.3f;

  float targetFeedback_ = 0.8f;
  float targetMix_ = 0.3f;
};

#endif // KAOSSEFFECT_REVERB_H
