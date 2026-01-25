#ifndef KAOSSEFFECT_GLITCH_H
#define KAOSSEFFECT_GLITCH_H

#include <vector>

class Glitch {
public:
  Glitch(int sampleRate);
  void process(float *buffer, int frames, int channels);
  void setParameters(float x, float y);
  void reset();

private:
  float sampleRate_;
  std::vector<float> buffer_;
  int writePos_ = 0;
  int bufferSize_ = 0;

  // State
  bool active_ = false;
  int readPos_ = 0;
  int grainSizeSamples_ = 0;
  int grainCounter_ = 0;

  // Parameters
  float targetRate_ = 0.5f; // X: Grain size (fast...slow)
  float targetMix_ = 0.0f;  // Y: Mix/Randomness

  float grainSizeTime_ = 0.1f; // 100ms
  float mix_ = 0.0f;
};

#endif // KAOSSEFFECT_GLITCH_H
