#include "essentia-session.hpp"
#include "audio-session.hpp"
#include "boost/uuid/string_generator.hpp"

NeonEssentiaSession::NeonEssentiaSession(const std::string audioSessID,
                                         const std::string sessID)
    : audioSessionID(audioSessID), sessionID(sessID), shutdown(false),
      algorithmFactory(essentia::streaming::AlgorithmFactory::instance()),
      logger("EssentiaSession-" + sessionID,
             DebugLogger::DebugColor::COLOR_MAGENTA, false) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Created Session");
}

NeonEssentiaSession::~NeonEssentiaSession() {
  shutdown = true;
  for (auto thread : threadPool) {
    if (thread->joinable())
      thread->join();

    // This might explode eventually... But we're shutting down anyways
    delete thread;
  }
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Destroyed Session");
}

[[nodiscard]] const std::string NeonEssentiaSession::GetSessionID() const
    noexcept {
  return sessionID;
}

void NeonEssentiaSession::createBPMPipeline(uint32_t newSampleRate,
                                            uint8_t newChannels,
                                            uint8_t newWidth,
                                            double newDuration) {
  int frameSize = 2048;
  int hopSize = 1024;

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Creating BPM Pipeline");

  auto rhythmExtractor =
      algorithmFactory.create("RhythmExtractor2013", "method", "multifeature");

  {
    std::scoped_lock<std::mutex> lock(algorithmMapMutex);
    algorithmMap["rhythmExtractor"] = rhythmExtractor;
  }

  essentia::streaming::Algorithm *root = nullptr;

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Connecting Algorithms");
  {
    std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);

    auto it = AudioSession::activeSessions.find(
        boost::uuids::string_generator()(audioSessionID));
    if (it == AudioSession::activeSessions.end()) {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                      "Unknown Session UUID [%s]", audioSessionID.c_str());
    } else {
      std::scoped_lock<std::mutex> lock(it->second->audioSinkMutex);
      root = it->second->getAudioSink();
      root->output("signal") >> rhythmExtractor->input("signal");
    }
  }

  rhythmExtractor->output("ticks") >>
      essentia::streaming::PoolConnector(pool, "rhythm.ticks");
  rhythmExtractor->output("confidence") >>
      essentia::streaming::PoolConnector(pool, "rhythm.confidence");
  rhythmExtractor->output("bpm") >>
      essentia::streaming::PoolConnector(pool, "rhythm.bpm");
  rhythmExtractor->output("estimates") >>
      essentia::streaming::PoolConnector(pool, "rhythm.estimates");
  rhythmExtractor->output("bpmIntervals") >>
      essentia::streaming::PoolConnector(pool, "rhythm.bpmIntervals");

  audioNetwork = new essentia::scheduler::Network(root);

  threadPool.push_back(new std::thread([this]() {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Starting BPM Worker thread");

    while (!shutdown) {
      audioNetwork->run();
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                      "BPM Worker thread step");
    }

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "BPM Worker thread exit");
  }));
}

void NeonEssentiaSession::getBPMPipeline(essentia::Real *bpm,
                                         essentia::Real *confidence) {
  const std::string rhythmKey = "rhythm.bpm";
  const std::string rhythmConfidence = "rhythm.confidence";

  // Maybe lock pool here with mutex?
  if (pool.contains<essentia::Real>(rhythmKey))
    *bpm = pool.value<essentia::Real>(rhythmKey);

  if (pool.contains<essentia::Real>(rhythmConfidence))
    *confidence = pool.value<essentia::Real>(rhythmConfidence) / 0.0532;
}