#pragma once

#include "debuglogger.hpp"
#include "essentia-session.hpp"
#include "neon.session.capnp.h"
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <essentia/streaming/algorithms/boostbuffer.h>
#include <memory>
#include <unordered_map>

class AudioSession {
public:
  AudioSession(const boost::uuids::uuid newUUID);
  ~AudioSession();

  const boost::uuids::uuid uuid;

  static std::mutex activeSessionMutex;
  static std::unordered_map<boost::uuids::uuid, std::shared_ptr<AudioSession>,
                            boost::hash<boost::uuids::uuid>>
      activeSessions;

  static std::mutex activePipelinesMutex;
  static std::unordered_map<boost::uuids::uuid,
                            std::shared_ptr<NeonEssentiaSession>,
                            boost::hash<boost::uuids::uuid>>
      activePipelines;

  // Setup the intial configuration
  void updateConfig(uint32_t newSampleRate, uint8_t newChannels,
                    uint8_t newWidth, double newDuration);

  const uint32_t getSampleRate() const noexcept;
  const uint8_t getChannels() const noexcept;
  const uint8_t getWidth() const noexcept;
  const double getDuration() const noexcept;
  const uint32_t getFrameSize() const noexcept;

  // push raw audio data into the pipeline
  std::mutex audioSinkMutex;
  essentia::streaming::BoostRingBufferInput *getAudioSink();

private:
  essentia::streaming::BoostRingBufferInput *audioData;

  uint32_t sampleRate;
  uint8_t channels;
  uint8_t width;
  double duration;
  uint32_t frameSize;

  DebugLogger logger;
};