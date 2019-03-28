#include "essentia-session.hpp"

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

// AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();

// Algorithm* audio = factory.create("MonoLoader",
//                                   "filename", audioFilename,
//                                   "sampleRate", sampleRate);

// Algorithm* fc    = factory.create("FrameCutter",
//                                   "frameSize", frameSize,
//                                   "hopSize", hopSize);

// Algorithm* w     = factory.create("Windowing",
//                                   "type", "blackmanharris62");

// Algorithm* spec  = factory.create("Spectrum");
// Algorithm* mfcc  = factory.create("MFCC");