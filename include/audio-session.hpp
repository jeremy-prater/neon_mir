#include "debuglogger.hpp"
#include <boost/uuid/uuid.hpp>

class AudioSession {
public:
  AudioSession(const char *data, const size_t size);
  ~AudioSession();

  const boost::uuids::uuid uuid;

private:
  uint8_t *audioData;
  DebugLogger logger;
};