#pragma once

#include "debuglogger.hpp"
#include <essentia/algorithmfactory.h>
#include <essentia/essentia.h>
#include <essentia/scheduler/network.h>
#include <essentia/streaming/algorithms/poolstorage.h>
#include <essentia/streaming/algorithms/vectorinput.h>
#include <thread>
#include <unordered_map>

class NeonEssentiaSession {
public:
  NeonEssentiaSession(const std::string audioSessID, const std::string sessID);
  ~NeonEssentiaSession();

  [[nodiscard]] const std::string GetSessionID() const noexcept;

  void createBPMPipeline(uint32_t newSampleRate, uint8_t newChannels,
                         uint8_t newWidth, double newDuration);

  void getBPMPipeline(essentia::Real *bpm, essentia::Real *confidence);

  void createSpectrumPipeline(uint32_t newSampleRate, uint8_t newChannels,
                              uint8_t newWidth, double newDuration);
  void getSpectrumData();

private:
  const std::string audioSessionID;
  const std::string sessionID;

  essentia::streaming::AlgorithmFactory &algorithmFactory;
  essentia::scheduler::Network *audioNetwork;
  essentia::Pool pool;
  essentia::Pool aggreatedPool;

  std::mutex algorithmMapMutex;
  std::unordered_map<std::string, essentia::streaming::Algorithm *>
      algorithmMap;
  std::unordered_map<std::string, essentia::standard::Algorithm *>
      algorithmStdMap;

  bool shutdown;
  std::vector<std::thread *> threadPool;

  DebugLogger logger;

  void updateAudioDataFromBuffer() noexcept;
};
