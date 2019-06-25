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
  auto windowing =
      algorithmFactory.create("Windowing", "type", "blackmanharris62");

  auto spectrum = algorithmFactory.create("Spectrum");

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

  spectrum->output("spectrum") >>
      essentia::streaming::PoolConnector(pool, "spectrum");

  audioNetwork = new essentia::scheduler::Network(root);

  // TODO : These should be defined by the client and
  // returned to the client...
  const char *stats[] = {"mean", "median", "min", "max"};

  essentia::standard::Algorithm *spectrumAggregator =
      essentia::standard::AlgorithmFactory::create(
          "PoolAggregator", "defaultStats",
          essentia::arrayToVector<std::string>(stats));

  spectrumAggregator->input("input").set(pool);
  spectrumAggregator->output("output").set(aggregatedPool);

  {
    std::scoped_lock<std::mutex> lock(algorithmMapMutex);
    algorithmMap["frameCutter"] = frameCutter;
    algorithmMap["windowing"] = windowing;
    algorithmMap["spectrum"] = spectrum;
    algorithmStdMap["spectrumAggregator"] = spectrumAggregator;
  }

  threadPool.push_back(new std::thread([this, root, spectrumAggregator]() {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Starting Spectrum Worker thread");

    while (!shutdown) {
      root->shouldStop(false);

      // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
      //                 "Starting Spectrum ==> Tick");

      try {
        audioNetwork->run();
      } catch (...) {
        logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                        "Spectrum Failed!!");
      }

      spectrumAggregator->compute();
      // usleep(100 * 1000);
    }

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Spectrum Worker thread exit");
  }));
}

void NeonEssentiaSession::getSpectrumData(
    ::neon::session::Controller::GetSpectrumDataResults::Builder &results) {
  const std::string spectrumKey = "spectrum";

  // auto poolKeys = aggregatedPool.descriptorNames();
  // for (auto key : poolKeys) {
  //   logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
  //                   "AggregratedPool contains key ==> %s", key.c_str());
  // }

  auto poolKeys = aggregatedPool.getRealPool();

// To queue bins or not to queue bins...

  for (auto key : poolKeys) {
    auto curKey = key.first;
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s ==> %d bins",
                    curKey.c_str(), key.second.size());
    if (curKey == "spectum.max")
    //setMax(::kj::ArrayPtr<const float> value);
      results.getData().setMax(
        kj::ArrayPtr()
        ::neon::session::SpectrumData::init
        key.second);
    else if (curKey == "spectum.max")
      results.getData().setMax(key.second);
    else if (curKey == "spectum.max")
      results.getData().setMax(key.second);
    else if (curKey == "spectum.max")
      results.getData().setMax(key.second);
    else {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                      "Unknown Key [%s]", curKey.c_str());
    }
  }
  aggregatedPool.clear();
}