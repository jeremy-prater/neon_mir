#pragma once

#include "debuglogger.hpp"
#include "essentia-session.hpp"
#include "neon.session.capnp.h"
#include <boost/circular_buffer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
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

  // Create audio processing pipelines

  // This might be dynamic...

  // Start the pipeline

  // Extract information about the current 'frame'

  // push raw audio data into the pipeline
  std::mutex audioSinkMutex;
  boost::circular_buffer<uint8_t> *getAudioSink();

private:
  boost::circular_buffer<uint8_t> audioData;

  uint32_t sampleRate;
  uint8_t channels;
  uint8_t width;
  double duration;
  
  DebugLogger logger;
};