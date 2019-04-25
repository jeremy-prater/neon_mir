#pragma once

#include "debuglogger.hpp"
#include "neon.session.capnp.h"
#include "essentia-session.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <memory>
#include <unordered_map>

class AudioSession {
public:
  AudioSession();
  ~AudioSession();

  const boost::uuids::uuid uuid;

  static std::mutex activeSessionMutex;
  static std::unordered_map<boost::uuids::uuid, std::shared_ptr<AudioSession>,
                            boost::hash<boost::uuids::uuid>>
      activeSessions;

  // Setup the intial configuration
  void updateConfig(uint32_t newSampleRate, uint8_t newChannels,
                    uint8_t newWidth, double newDuration);

  // Create audio processing pipelines

  // This might be dynamic...

  // Start the pipeline

  // Extract information about the current 'frame'

  // push raw audio data into the pipeline
  std::mutex audioSinkMutex;
  boost::circular_buffer<uint8_t> *getAudioSink();

  NeonEssentiaSession essentiaSession;

private:
  boost::circular_buffer<uint8_t> audioData;
  DebugLogger logger;

  uint32_t sampleRate;
  uint8_t channels;
  uint8_t width;
  double duration;
};