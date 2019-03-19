#pragma once

#include "debuglogger.hpp"
#include "essentia.h"

using namespace essentia::streaming;

class NeonEssentia {
public:
  NeonEssentia();
  ~NeonEssentia();

  Pool pool;

  const Real sampleRate = 44100.0;
  const int frameSize = 2048;
  const int hopSize = 1024;

private:
  DebugLogger logger;
};
