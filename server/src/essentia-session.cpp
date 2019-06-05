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

  audioVectorData.clear();

  if (it == AudioSession::activeSessions.end()) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Unknown Session UUID [%s]", audioSessionID.c_str());
  } else {
    auto audioSession = it->second;
    std::scoped_lock<std::mutex> lock(audioSession->audioSinkMutex);

    auto circlarBuffer = audioSession->getAudioSink();

    switch (audioSession->getWidth()) {
    case 8: {
      for (auto value : *circlarBuffer)
        audioVectorData.push_back(value);

    } break;
    case 16: {
      auto it = circlarBuffer->begin();
      while (it != circlarBuffer->end()) {
        int16_t value = 0;
        value |= *it++ << 0;
        value |= *it++ << 8;
        if (audioSession->getChannels() == 2) {
          it++;
          it++;
        }
        essentia::Real rValue = static_cast<essentia::Real>(value) / SHRT_MAX;
        audioVectorData.push_back(rValue);
      }
    } break;
    default:
      break;
    }

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Samples in buffer [%d]", audioVectorData.size());
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
      algorithmFactory.create("RhythmExtractor2013", "method", "multifeature");

  {
    std::scoped_lock<std::mutex> lock(algorithmMapMutex);
    algorithmMap["audioVectorInput"] = audioVectorInput;
    algorithmMap["rhythmExtractor"] = rhythmExtractor;
  }

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Connecting Algorithms");

  audioVectorInput->output("data") >> rhythmExtractor->input("signal");
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

  audioNetwork = new essentia::scheduler::Network(audioVectorInput);
}

void NeonEssentiaSession::runBPMPipeline(essentia::Real *bpm,
                                         essentia::Real *confidence) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Running BPM Pipeline");

  updateAudioDataFromBuffer();

  if (!audioVectorData.size())
    return;

  audioVectorInput->setVector(&audioVectorData);
  audioNetwork->run();

  essentia::Real newBpm = pool.value<essentia::Real>("rhythm.bpm");
  essentia::Real newConfidence =
      pool.value<essentia::Real>("rhythm.confidence") / 0.0532;

  *bpm = newBpm;
  *confidence = newConfidence;

  pool.clear();
  audioNetwork->reset();
}