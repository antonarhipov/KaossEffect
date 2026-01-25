#include "AudioEngine.h"
#include <android/log.h>
#include <cmath>

#define TAG "AudioEngine"

AudioEngine *AudioEngine::getInstance() {
  static AudioEngine engine;
  return &engine;
}

AudioEngine::~AudioEngine() { stopStream(); }

AudioEngine::AudioEngine() {
  filter_ = std::make_unique<Filter>(kSampleRate);
  chorus_ = std::make_unique<Chorus>(kSampleRate);
  reverb_ = std::make_unique<Reverb>(kSampleRate);
  phaser_ = std::make_unique<Phaser>(kSampleRate);
  bitcrusher_ = std::make_unique<Bitcrusher>(kSampleRate);
  ringMod_ = std::make_unique<RingMod>(kSampleRate);
}

// Lifecycle methods
void AudioEngine::startStream() {
  oboe::AudioStreamBuilder builder;
  builder.setDirection(oboe::Direction::Output);
  builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
  builder.setSharingMode(oboe::SharingMode::Shared);
  builder.setFormat(oboe::AudioFormat::Float);
  builder.setChannelCount(kChannelCount);
  builder.setSampleRate(kSampleRate);
  builder.setCallback(this);

  oboe::Result result = builder.openStream(stream_);
  if (result != oboe::Result::OK) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to open stream: %s",
                        oboe::convertToText(result));
    return;
  }

  result = stream_->requestStart();
  if (result != oboe::Result::OK) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to start stream: %s",
                        oboe::convertToText(result));
    stream_->close();
    return;
  }

  __android_log_print(ANDROID_LOG_INFO, TAG, "Stream started");
}

void AudioEngine::stopStream() {
  if (stream_) {
    stream_->stop();
    stream_->close();
    stream_.reset();
  }
  __android_log_print(ANDROID_LOG_INFO, TAG, "Stream stopped");
}

void AudioEngine::setXY(float x, float y) {
  paramX_.store(x);
  paramY_.store(y);
}

void AudioEngine::setEffectMode(int mode) { effectMode_.store(mode); }

bool AudioEngine::isPlaying() { return isPlaying_.load(); }

bool AudioEngine::loadFile(int fd, int64_t offset, int64_t size) {
  std::lock_guard<std::mutex> lock(decoderMutex_);
  bool wasPlaying = isPlaying_.load();
  isPlaying_.store(false);

  auto newDecoder = std::make_unique<Mp3Decoder>();
  if (newDecoder->open(fd, offset, size)) {
    decoder_ = std::move(newDecoder);
    __android_log_print(ANDROID_LOG_INFO, TAG,
                        "File loaded successfully. Duration: %lld",
                        decoder_->getDurationMs());
    return true;
  }
  __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to open mp3 decoder");
  return false;
}

void AudioEngine::play() {
  isPlaying_.store(true);
  __android_log_print(ANDROID_LOG_INFO, TAG,
                      "Play requested. isPlaying = true");
}

void AudioEngine::pause() {
  isPlaying_.store(false);
  __android_log_print(ANDROID_LOG_INFO, TAG,
                      "Pause requested. isPlaying = false");
}

// Transport Stop: Stop playback and rewind
void AudioEngine::stop() {
  isPlaying_.store(false);
  seekTo(0);
  __android_log_print(ANDROID_LOG_INFO, TAG, "Stop requested.");
}

void AudioEngine::seekTo(int64_t positionMs) {
  std::lock_guard<std::mutex> lock(decoderMutex_);
  if (decoder_) {
    decoder_->seekTo(positionMs);
  }
}

int64_t AudioEngine::getDurationMs() {
  std::lock_guard<std::mutex> lock(decoderMutex_);
  if (decoder_) {
    return decoder_->getDurationMs();
  }
  return 0;
}

int64_t AudioEngine::getPositionMs() {
  // No lock needed for simple atomic read if we cached it,
  // but decoder access needs lock or atomic wrapper.
  // For now, lock to be safe.
  std::lock_guard<std::mutex> lock(decoderMutex_);
  if (decoder_) {
    return decoder_->getPositionMs();
  }
  return 0;
}

oboe::DataCallbackResult
AudioEngine::onAudioReady(oboe::AudioStream *audioStream, void *audioData,
                          int32_t numFrames) {
  auto *outputData = static_cast<float *>(audioData);

  // Default to silence
  std::fill(outputData, outputData + numFrames * kChannelCount, 0.0f);

  if (!isPlaying_.load())
    return oboe::DataCallbackResult::Continue;

  std::unique_lock<std::mutex> lock(decoderMutex_, std::try_to_lock);
  if (!lock.owns_lock() || !decoder_) {
    // Output silence if locked (loading file) or no decoder
    return oboe::DataCallbackResult::Continue;
  }

  int samplesRead = decoder_->read(outputData, numFrames * kChannelCount);

  // Debug logging for starvation (throttled)
  static int starvationLogCounter = 0;
  if (samplesRead == 0 && !decoder_->isEndOfStream()) {
    starvationLogCounter++;
    if (starvationLogCounter % 100 ==
        1) { // Log every ~1s (assuming 10ms-ish callback)
      __android_log_print(ANDROID_LOG_WARN, TAG,
                          "Starvation detected: read 0 samples but not EOS");
    }
  } else {
    starvationLogCounter = 0;
  }

  if (samplesRead < numFrames * kChannelCount) {
    if (decoder_->isEndOfStream()) {
      isPlaying_.store(false);
      __android_log_print(ANDROID_LOG_INFO, TAG, "Playback finished (EOS)");
    }
  }

  // Effect processing
  int mode = effectMode_.load();
  float x = paramX_.load();
  float y = paramY_.load();

  if (mode == 0 && filter_) { // Filter
    filter_->setParameters(x, y);
    filter_->process(outputData, samplesRead / kChannelCount, kChannelCount);
  } else if (mode == 1 && chorus_) { // Chorus
    chorus_->setParameters(x, y);
    chorus_->process(outputData, samplesRead / kChannelCount, kChannelCount);
  } else if (mode == 2 && reverb_) { // Reverb
    reverb_->setParameters(x, y);
    reverb_->process(outputData, samplesRead / kChannelCount, kChannelCount);
  } else if (mode == 3 && phaser_) { // Phaser
    phaser_->setParameters(x, y);
    phaser_->process(outputData, samplesRead / kChannelCount, kChannelCount);
  } else if (mode == 4 && bitcrusher_) { // Bitcrusher
    bitcrusher_->setParameters(x, y);
    bitcrusher_->process(outputData, samplesRead / kChannelCount,
                         kChannelCount);
  } else if (mode == 5 && ringMod_) { // RingMod
    ringMod_->setParameters(x, y);
    ringMod_->process(outputData, samplesRead / kChannelCount, kChannelCount);
  } else {
    // Passthrough
  }

  // Soft Clipper (tanh-like limiting) to prevent harsh digital clipping
  // simple polynomial: f(x) = x - x^3/3 for x in [-1.5, 1.5] roughly covers it,
  // or just hard clamp after some saturation.
  // We'll use a fast saturation curve: x / (1 + |x|) is simple but effective.
  // Or std::tanh which sounds nice.
  for (int i = 0; i < numFrames * kChannelCount; ++i) {
    float x = outputData[i];
    // Fast sigmoid: x / (1 + |x|) approaches +/- 1.0 asymptotically
    // Let's use std::tanh for "warm" overdrive behavior
    // outputData[i] = std::tanh(x);

    // Actually, let's use a slightly harder knee to preserve volume until it
    // hits the limit. If x > 1.0, clamp. But we want to avoid hard clip. Let's
    // stick to std::clamp for safety first, effectively hard limiting, but
    // maybe a little soft knee? Let's use simple hard clamping for now to
    // strictly satisfy "prevent harsh clipping" (which usually means
    // wrapping/overflow). But user asked for "soft clipper". Simple soft clip:
    // if (x > 1.0f) x = 1.0f; but that's hard clip. x = x < -1.5f ? -1.0f : (x
    // > 1.5f ? 1.0f : ...);

    // Standard cubic soft clipper
    if (x < -1.5f) {
      x = -1.0f;
    } else if (x > 1.5f) {
      x = 1.0f;
    } else {
      x = x - (x * x * x) / 27.0f; // Soft saturation up to 1.5 input
      // wait, derivative at 1.5 is 1 - 3*1.5^2/27 = 1 - 6.75/27 = 0.75?
      // The formula x - x^3/3 is standard for [-1, 1].
    }

    // Let's just use std::tanh, it's reliable and sounds good.
    outputData[i] = std::tanh(x);
  }

  return oboe::DataCallbackResult::Continue;
}
