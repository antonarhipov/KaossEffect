#ifndef KAOSSEFFECT_RINGMOD_H
#define KAOSSEFFECT_RINGMOD_H

class RingMod {
public:
  RingMod(int sampleRate);
  void process(float *buffer, int frames, int channels);
  void setParameters(float x, float y);
  void reset();

private:
  float sampleRate_;

  float carrierPhase_ = 0.0f;
  float carrierFreq_ = 440.0f;
  float mix_ = 0.0f;

  float targetFreq_ = 440.0f;
  float targetMix_ = 0.0f;
};

#endif // KAOSSEFFECT_RINGMOD_H
