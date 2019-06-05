#include "essentia-session.hpp"
#include "audio-session.hpp"
#include "boost/uuid/string_generator.hpp"

NeonEssentiaSession::NeonEssentiaSession(const std::string audioSessID,
                                         const std::string sessID)
    : audioSessionID(audioSessID), sessionID(sessID),
      algorithmFactory(essentia::streaming::AlgorithmFactory::instance()),
      logger("EssentiaSession-" + sessionID,
             DebugLogger::DebugColor::COLOR_MAGENTA, false) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Created Session");
}

NeonEssentiaSession::~NeonEssentiaSession() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Destroyed Session");
}

[[nodiscard]] const std::string NeonEssentiaSession::GetSessionID() const
    noexcept {
  return sessionID;
}

void NeonEssentiaSession::updateAudioDataFromBuffer() noexcept {
  auto it = AudioSession::activeSessions.find(
      boost::uuids::string_generator()(audioSessionID));

  if (it == AudioSession::activeSessions.end()) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Unknown Session UUID [%s]", audioSessionID.c_str());
  } else {
    std::scoped_lock<std::mutex> lock(it->second->audioSinkMutex);

    auto circlarBuffer = it->second->getAudioSink();
    for (auto value : *circlarBuffer)
      audioVectorData.push_back(value);

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "sizes [%d][%d]",
                    circlarBuffer->size(), audioVectorData.size());
  }
}

void NeonEssentiaSession::createBPMPipeline(uint32_t newSampleRate,
                                            uint8_t newChannels,
                                            uint8_t newWidth,
                                            double newDuration) {
  int frameSize = 2048;
  int hopSize = 1024;

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Creating BPM Pipeline");

  audioVectorInput =
      new essentia::streaming::VectorInput<essentia::Real>(&audioVectorData);

  auto rhythmExtractor =
      algorithmFactory.create("rhythmExtractor2013", "method", "multifeature");

  {
    std::scoped_lock<std::mutex> lock(algorithmMapMutex);
    algorithmMap["audioVectorInput"] = audioVectorInput;
    algorithmMap["rhythmExtractor"] = rhythmExtractor;
  }

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Connecting Algorithms");

  essentia::streaming::connect(*audioVectorInput,
                               rhythmExtractor->input("signal"));

  rhythmExtractor->output("ticks") >>
      essentia::streaming::PoolConnector(pool, "rhythm.ticks");
  rhythmExtractor->output("confidence") >>
      essentia::streaming::PoolConnector(pool, "rhythm.ticks_confidence");
  rhythmExtractor->output("bpm") >>
      essentia::streaming::PoolConnector(pool, "rhythm.bpm");
  rhythmExtractor->output("estimates") >>
      essentia::streaming::PoolConnector(pool, "rhythm.estimates");
  rhythmExtractor->output("bpmIntervals") >>
      essentia::streaming::PoolConnector(pool, "rhythm.bpmIntervals");

  audioNetwork = new essentia::scheduler::Network(audioVectorInput);
}

void NeonEssentiaSession::runBPMPipeline(essentia::Real *bpm,
                                         essentia::Real *confidence) {
  if (!audioVectorData.size() < 4096)
    return;

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Running Algorithms");
  audioNetwork->run();

  *bpm = pool.value<essentia::Real>("rhythm.bpm"),
  *confidence = pool.value<essentia::Real>("rhythm.ticks_confidence") / 0.0532;

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Results : BPM [%f] Confidence [%f%%]", bpm, confidence);
}