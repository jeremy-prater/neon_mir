#pragma once

#include "debuglogger.hpp"
#include <essentia/algorithmfactory.h>
#include <essentia/essentia.h>
#include <essentia/scheduler/network.h>
#include <essentia/streaming/algorithms/poolstorage.h>
#include <essentia/streaming/algorithms/vectorinput.h>
#include <unordered_map>

class NeonEssentiaSession {
public:
  NeonEssentiaSession(const std::string audioSessID, const std::string sessID);
  ~NeonEssentiaSession();

  [[nodiscard]] const std::string GetSessionID() const noexcept;

  void createBPMPipeline(uint32_t newSampleRate, uint8_t newChannels,
                         uint8_t newWidth, double newDuration);

  void runBPMPipeline(essentia::Real *bpm, essentia::Real *confidence);

private:
  const std::string audioSessionID;
  const std::string sessionID;

  std::vector<essentia::Real> audioVectorData;
  essentia::streaming::VectorInput<essentia::Real> *audioVectorInput;
  essentia::streaming::AlgorithmFactory &algorithmFactory;
  essentia::scheduler::Network *audioNetwork;
  essentia::Pool pool;

  std::mutex algorithmMapMutex;
  std::unordered_map<std::string, essentia::streaming::Algorithm *>
      algorithmMap;

  DebugLogger logger;

  void updateAudioDataFromBuffer() noexcept;
};
