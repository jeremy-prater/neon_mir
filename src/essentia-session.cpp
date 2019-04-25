#include "essentia-session.hpp"
#include <essentia/algorithmfactory.h>
#include <essentia/essentia.h>
#include <essentia/scheduler/network.h>
#include <essentia/streaming/algorithms/poolstorage.h>

NeonEssentiaSession::NeonEssentiaSession(const std::string sessID)
    : logger("EssentiaSession-" + sessID,
             DebugLogger::DebugColor::COLOR_MAGENTA, false),
      sessionID(sessID) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Created Session");
}

NeonEssentiaSession::~NeonEssentiaSession() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Destroyed Session");
}

[[nodiscard]] const std::string NeonEssentiaSession::GetSessionID() const
    noexcept {
  return sessionID;
}

void NeonEssentiaSession::defaultConfig(uint32_t newSampleRate,
                                        uint8_t newChannels, uint8_t newWidth,
                                        double newDuration) {
  int frameSize = 2048;
  int hopSize = 1024;

  essentia::Pool pool;

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Creating Algorithms");

  essentia::streaming::AlgorithmFactory &factory =
      essentia::streaming::AlgorithmFactory::instance();

  Algorithm *audio = factory.create(
      "MonoLoader", "filename",
      "/home/prater/src/neon_mir/samples/midnight-drive-clip-10s.wav",
      "sampleRate", newSampleRate);

  Algorithm *rhythmextractor =
      factory.create("RhythmExtractor2013", "method", "multifeature");

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Connecting Algorithms");

  audio->output("audio") >> rhythmextractor->input("signal");

  rhythmextractor->output("ticks") >>
      essentia::streaming::PoolConnector(pool, "rhythm.ticks");
  rhythmextractor->output("confidence") >>
      essentia::streaming::PoolConnector(pool, "rhythm.ticks_confidence");
  rhythmextractor->output("bpm") >>
      essentia::streaming::PoolConnector(pool, "rhythm.bpm");
  rhythmextractor->output("estimates") >>
      essentia::streaming::PoolConnector(pool, "rhythm.estimates");
  rhythmextractor->output("bpmIntervals") >>
      essentia::streaming::PoolConnector(pool, "rhythm.bpmIntervals");

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Running Algorithms");

  essentia::scheduler::Network network(audio);
  network.run();

  logger.WriteLog(
      DebugLogger::DebugLevel::DEBUG_INFO, "Results : BPM [%f] Confidence [%f%%]",
      pool.value<essentia::Real>("rhythm.bpm"),
      pool.value<essentia::Real>("rhythm.ticks_confidence") / 0.0532);
}