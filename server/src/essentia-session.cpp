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
    }
  }

  root->output("signal") >> rhythmExtractor->input("signal");

  rhythmExtractor->input("signal").setAcquireSize(newChannels * (newWidth / 8) *
                                                  newSampleRate * newDuration);

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

  threadPool.push_back(new std::thread([this, root, rhythmExtractor]() {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Starting BPM Worker thread");

    while (!shutdown) {
      root->shouldStop(false);

      // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
      //                 "Starting BPM ==> Tick");

      try {
        std::scoped_lock<std::mutex> lock(poolMutex);
        audioNetwork->run();
      } catch (...) {
        // Reset the extractor
        rhythmExtractor->reset();
        // Try again...
        audioNetwork->run();
      }

      // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
      //                 "Starting BPM ==> Tock");

      usleep(100 * 1000);
    }

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "BPM Worker thread exit");
  }));
}

void NeonEssentiaSession::getBPMPipeline(essentia::Real *bpm,
                                         essentia::Real *confidence) {
  std::scoped_lock<std::mutex> lock(poolMutex);
  const std::string rhythmKey = "rhythm.bpm";
  const std::string rhythmConfidence = "rhythm.confidence";

  // Maybe lock pool here with mutex?
  if (pool.contains<essentia::Real>(rhythmKey))
    *bpm = pool.value<essentia::Real>(rhythmKey);
  else
    *bpm = 0;

  if (pool.contains<essentia::Real>(rhythmConfidence))
    *confidence = pool.value<essentia::Real>(rhythmConfidence) / 0.0532;
  else
    *confidence = 0;
}

void NeonEssentiaSession::createSpectrumPipeline(uint32_t newSampleRate,
                                                 uint8_t newChannels,
                                                 uint8_t newWidth,
                                                 double newDuration) {
  int frameSize = 1024; // TODO : These should be tunable
  int hopSize = 512;

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Creating Spectrum Pipeline");

  auto frameCutter = algorithmFactory.create("FrameCutter", "frameSize",
                                             frameSize, "hopSize", hopSize);
  auto windowing = algorithmFactory.create("Windowing", "type", "hann");

  auto spectrum = algorithmFactory.create("Spectrum");
  auto logSpectrum =
      algorithmFactory.create("LogSpectrum", "frameSize", frameSize);
  // auto nnls = algorithmFactory.create("NNLSChroma", "frameSize", frameSize,
  //                                     "sampleRate", newSampleRate);

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
    }
  }

  // Audio -> FrameCutter
  root->output("signal") >> frameCutter->input("signal");

  // FrameCutter -> Windowing -> Spectrum
  frameCutter->output("frame") >> windowing->input("frame");

  windowing->output("frame") >> spectrum->input("frame");

  spectrum->output("spectrum") >> logSpectrum->input("spectrum");

  logSpectrum->output("meanTuning") >> essentia::streaming::NOWHERE;
  logSpectrum->output("localTuning") >> essentia::streaming::NOWHERE;
  logSpectrum->output("logFreqSpectrum") >>
      essentia::streaming::PoolConnector(pool, "spectrum");

  // logSpectrum->output("meanTuning") >> nnls->input("meanTuning");
  // logSpectrum->output("localTuning") >> nnls->input("localTuning");
  // logSpectrum->output("logFreqSpectrum") >> nnls->input("logSpectrogram");

  // nnls->output("tunedLogfreqSpectrum") >>
  //     essentia::streaming::PoolConnector(pool, "tunedLogfreqSpectrum");
  // nnls->output("semitoneSpectrum") >>
  //     essentia::streaming::PoolConnector(pool, "semitoneSpectrum");
  // nnls->output("bassChromagram") >>
  //     essentia::streaming::PoolConnector(pool, "bassChromagram");
  // nnls->output("chromagram") >>
  //     essentia::streaming::PoolConnector(pool, "chromagram");

  audioNetwork = new essentia::scheduler::Network(root);

  {
    std::scoped_lock<std::mutex> lock(algorithmMapMutex);
    algorithmMap["frameCutter"] = frameCutter;
    algorithmMap["windowing"] = windowing;
    algorithmMap["spectrum"] = spectrum;
    algorithmMap["logSpectrum"] = logSpectrum;
    // algorithmMap["nnls"] = nnls;
  }

  threadPool.push_back(new std::thread([this, root]() {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Starting Spectrum Worker thread");

    while (!shutdown) {
      root->shouldStop(false);

      // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
      //                 "Starting Spectrum ==> Tick");

      try {
        std::scoped_lock<std::mutex> lock(poolMutex);
        audioNetwork->run();
      } catch (...) {
        logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                        "Spectrum Failed!!");
      }

      usleep(50 * 1000);
    }

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Spectrum Worker thread exit");
  }));
}

void NeonEssentiaSession::getSpectrumData(
    ::neon::session::Controller::GetSpectrumDataResults::Builder &results) {
  static essentia::Real lastChunk[256];

  // Maybe lock pool here with mutex?
  std::scoped_lock<std::mutex> lock(poolMutex);

  // Things that are important
  const std::string spectrumKey = "spectrum";

  memset(lastChunk, 0, sizeof(lastChunk));

  // Maybe lock pool here with mutex?
  if (pool.contains<std::vector<std::vector<essentia::Real>>>(spectrumKey)) {
    auto dataChunks =
        pool.value<std::vector<std::vector<essentia::Real>>>(spectrumKey);

    // Aggregrate the frame into one...
    // O(n^2) :(

    auto numChunks = dataChunks.size();
    for (auto chunk : dataChunks) {
      const size_t depth = chunk.size();
      for (size_t index = 0; index < depth; index++) {
        lastChunk[index] += chunk[index] / numChunks;
      }
    }

    // TODO: This could be bad multi threading...
    // Whenwe go out of scope, the mutex is released...
    results.getData().setRaw(kj::arrayPtr(lastChunk, 256));
    pool.clear();
  }
}