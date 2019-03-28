#pragma once

#include "debuglogger.hpp"
#include "essentia/essentia.h"

using namespace essentia::streaming;

class NeonEssentiaSession {
public:
  NeonEssentiaSession(const std::string sessID);
  ~NeonEssentiaSession();

  [[nodiscard]] const std::string GetSessionID() const noexcept;

private:
  DebugLogger logger;
  const std::string sessionID;
};

