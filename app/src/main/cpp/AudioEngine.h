#ifndef KAOSSEFFECT_AUDIOENGINE_H
#define KAOSSEFFECT_AUDIOENGINE_H

#include "Mp3Decoder.h"
#include "effects/Bitcrusher.h"
#include "effects/Chorus.h"
#include "effects/Filter.h"
#include "effects/Glitch.h"
#include "effects/Phaser.h"
#include "effects/Reverb.h"
#include "effects/RingMod.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <oboe/Oboe.h>

class AudioEngine : public oboe::AudioStreamCallback {
public:
  static AudioEngine *getInstance();

  void startStream();
  void stopStream();
  void setXY(float x, float y);
  void setEffectMode(int mode);
  bool isPlaying();

  // Playback controls
  bool loadFile(int fd, int64_t offset, int64_t size);
  void play();
  void pause();
  void stop();
  void seekTo(int64_t positionMs);
  int64_t getDurationMs();
  int64_t getPositionMs();

  oboe::DataCallbackResult onAudioReady(oboe::AudioStream *audioStream,
                                        void *audioData,
                                        int32_t numFrames) override;

private:
  AudioEngine();
  ~AudioEngine();

  std::shared_ptr<oboe::AudioStream> stream_;
  std::atomic<float> paramX_{0.5f};
  std::atomic<float> paramY_{0.5f};
  std::atomic<int> effectMode_{0};
  std::atomic<bool> isPlaying_{false};

  // Decoder
  std::unique_ptr<Mp3Decoder> decoder_;
  std::mutex decoderMutex_;

  // Effects
  std::unique_ptr<Filter> filter_;
  std::unique_ptr<Chorus> chorus_;
  std::unique_ptr<Reverb> reverb_;
  std::unique_ptr<Phaser> phaser_;
  std::unique_ptr<Bitcrusher> bitcrusher_;
  std::unique_ptr<RingMod> ringMod_;

  // Test tone phase
  float phase_ = 0.0f;

  static constexpr int kSampleRate = 44100;
  static constexpr int kChannelCount = 2;
};

#endif // KAOSSEFFECT_AUDIOENGINE_H
