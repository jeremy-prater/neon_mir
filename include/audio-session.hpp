#pragma once

#include "debuglogger.hpp"
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

  void updateConfig(uint32_t newSampleRate, uint8_t newChannels,
                    uint8_t newWidth, double newDuration);

private:
  std::shared_ptr<uint8_t> audioData;
  DebugLogger logger;

  uint32_t sampleRate;
  uint8_t channels;
  uint8_t width;
  double duration;
};