#pragma once

#include "debuglogger.hpp"
#include "essentia/essentia.h"

using namespace essentia::streaming;

class NeonEssentiaSession {
public:
  NeonEssentiaSession(const std::string sessID);
  ~NeonEssentiaSession();

  [[nodiscard]] const std::string GetSessionID() const noexcept;

  void defaultConfig(uint32_t newSampleRate, uint8_t newChannels,
                    uint8_t newWidth, double newDuration);

private:
  DebugLogger logger;
  const std::string sessionID;
};
