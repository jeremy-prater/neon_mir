#include "audio-processor.hpp"

//////////////////////////////////////////////////////////////////////
//
// Audio processor implementation
//

static std::string spectrumUUID;

AudioProcessor::AudioProcessor()
    : handle("test-pipeline"), defaultSampleRate(44100), defaultChannels(1),
      defaultWidth(16), defaultDurationMs(1 * 1000),
      audioProcessorThreadRunning(true),
      logger("AudioProcessor-", DebugLogger::DebugColor::COLOR_RED, false) {
  audioProcessorThread = std::thread(&AudioProcessor::audioProcessorLoop, this);
}

AudioProcessor::~AudioProcessor() {
  audioProcessorThreadRunning = false;
  audioProcessorWakeup.notify_all();
  if (audioProcessorThread.joinable())
    audioProcessorThread.join();
}

void AudioProcessor::audioProcessorLoop() {
  capnp::EzRpcClient client("localhost:5554");
  neon::session::Controller::Client controllerServer =
      client.getMain<neon::session::Controller>();
  kj::WaitScope &waitScope = client.getWaitScope();

  {
    auto request = controllerServer.createSessionRequest();
    request.setName(handle);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
    sessionUUID = response.getUuid();
  }

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Created AudioProcessor Thread ==> Essentia UUID [%s]",
                  sessionUUID.c_str());

  {
    auto request = controllerServer.updateSessionConfigRequest();
    request.getConfig().setUuid(sessionUUID);
    request.getConfig().setSampleRate(defaultSampleRate);
    request.getConfig().setChannels(defaultChannels);
    request.getConfig().setWidth(defaultWidth);
    request.getConfig().setDuration(defaultDurationMs);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
  }

  {
    auto request = controllerServer.createSpectrumPipeRequest();
    request.setUuid(sessionUUID);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
    spectrumUUID = response.getUuid();
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Created Spectrum Pipeline [%s]", spectrumUUID.c_str());
  }
  int count = 0;

  while (audioProcessorThreadRunning) {
    // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
    //                 "AudioProcessor Thread Loop Sleep");

    std::unique_lock<std::mutex> waitLock(audioProcessorWakeupMutex);
    audioProcessorWakeup.wait(waitLock);

    // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
    //                 "AudioProcessor Thread Loop Woke up");

    // Write music sample... in 1024 byte chunks?
    {
      std::scoped_lock<std::mutex> lock(audioQueueMutex);
      for (auto audioChunk : audioQueue) {
        float *audioChunkStart = &audioChunk.front();
        size_t bytesLeft = audioChunk.size();
        size_t offset = 0;

        while (bytesLeft) {
          size_t sendSize = std::min(4096, static_cast<int>(bytesLeft));
          auto request = controllerServer.pushAudioDataRequest();
          request.getData().setUuid(sessionUUID);
          kj::ArrayPtr<const float> arrayPtr(audioChunkStart + offset,
                                             sendSize);
          request.getData().setSegment(arrayPtr);

          offset += sendSize;
          bytesLeft -= sendSize;

          auto promise = request.send();
          auto response = promise.wait(waitScope);
        }
      }
      audioQueue.clear();
    }

    {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Get Spectrum Data");

      auto request = controllerServer.getSpectrumDataRequest();
      request.setUuid(spectrumUUID);
      auto promise = request.send();
      auto response = promise.wait(waitScope);
    }
  }
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "AudioProcessor Thread Exited");
}

[[nodiscard]] const uint32_t AudioProcessor::getSampleRate() const noexcept {
  return defaultSampleRate;
}

[[nodiscard]] const uint8_t AudioProcessor::getChannels() const noexcept {
  return defaultChannels;
}

[[nodiscard]] const uint8_t AudioProcessor::getWidth() const noexcept {
  return defaultWidth;
}

[[nodiscard]] const uint32_t AudioProcessor::getDurationMs() const noexcept {
  return defaultDurationMs;
}

void AudioProcessor::processAudio(const float *musicData,
                                  const size_t musicDataLength) const noexcept {
  // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Processing Chunk
  // [%d]",
  //                 musicDataLength / 4);

  for (size_t index = 0; index < musicDataLength; index++) {
    const float value = musicData[index];
    if ((value < -1) || (value > 1)) {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_ERROR,
                      "Incoming Audio data out of range [-1, 1] ==> %f", value);
      assert(0);
    } else {
      // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
      //                 "Incoming Audio data in range ==> %f", value);
    }
  }
  {
    std::scoped_lock<std::mutex> lock(audioQueueMutex);
    audioQueue.push_back(
        std::vector<float>(musicData, musicData + musicDataLength));
  }

  audioProcessorWakeup.notify_all();
}