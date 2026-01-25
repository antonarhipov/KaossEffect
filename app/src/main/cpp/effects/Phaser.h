#ifndef KAOSSEFFECT_PHASER_H
#define KAOSSEFFECT_PHASER_H

class Phaser {
public:
  Phaser(int sampleRate);
  void process(float *buffer, int frames, int channels);
  void setParameters(float x, float y);
  void reset();

private:
  float sampleRate_;

  struct AllpassStage {
    float oldInput = 0.0f;
    float oldOutput = 0.0f;

    float process(float input, float allpassCoeff) {
      float output =
          input * -allpassCoeff + oldInput + oldOutput * allpassCoeff;
      oldInput = input;
      oldOutput = output;
      return output;
    }
  };

  // 6 stages for deep phasing
  AllpassStage stages_[6][2]; // Stereo stages

  float lfoPhase_ = 0.0f;
  float lfoRate_ = 0.5f;
  float depth_ = 0.5f;
  float feedback_ = 0.0f;
  float lastOutput_[2] = {0.0f, 0.0f}; // Stereo feedback storage

  float targetRate_ = 0.5f;
  float targetDepth_ = 0.5f;
};

#endif // KAOSSEFFECT_PHASER_H
