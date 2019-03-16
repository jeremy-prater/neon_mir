#pragma once

#include "debuglogger.hpp"
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <memory>
#include <unordered_map>

class AudioSession {
public:
  AudioSession(const char *data, const size_t size);
  ~AudioSession();

  const boost::uuids::uuid uuid;

  static std::mutex activeSessionMutex;
  static std::unordered_map<boost::uuids::uuid, std::shared_ptr<AudioSession>,
                            boost::hash<boost::uuids::uuid>>
      activeSessions;

private:
  uint8_t *audioData;
  DebugLogger logger;
};