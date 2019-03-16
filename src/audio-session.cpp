#include "audio-session.hpp"
#include "memory.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <stdlib.h>

std::mutex AudioSession::activeSessionMutex;
std::unordered_map<boost::uuids::uuid, std::shared_ptr<AudioSession>,
                   boost::hash<boost::uuids::uuid>>
    AudioSession::activeSessions;

AudioSession::AudioSession(const char *data, const size_t size)
    // : uuid(boost::uuids::random_generator()()),
    : uuid(boost::uuids::string_generator()("aebe3b0e-a12d-4a30-82f7-ad20df2cc3d5")),
      logger("AudioSession-" + boost::uuids::to_string(uuid),
             DebugLogger::DebugColor::COLOR_BLUE, true) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Creating Session from [%d] bytes of audio data!", size);

  audioData = static_cast<uint8_t *>(malloc(size));
  memcpy(audioData, data, size);
}

AudioSession::~AudioSession() {
  free(audioData);
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Destoryed Session!");
}
