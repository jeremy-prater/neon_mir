#include "audio-processor.hpp"

//////////////////////////////////////////////////////////////////////
//
// Audio processor implementation
//

static std::string bpmUUID;

AudioProcessor::AudioProcessor()
    : handle("test-pipeline"), defaultSampleRate(44100), defaultChannels(2),
      defaultWidth(16), defaultDurationMs(10 * 1000),
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
    auto config = controllerServer.updateSessionConfigRequest().initConfig();
    config.setUuid(sessionUUID);
    config.setSampleRate(defaultSampleRate);
    config.setChannels(defaultChannels);
    config.setWidth(defaultWidth);
    config.setDuration(defaultDurationMs);
    request.setConfig(config);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
  }

  {
    auto request = controllerServer.createBPMPipeLineRequest();
    request.setUuid(sessionUUID);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
    bpmUUID = response.getUuid();
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Created BPM Pipeline [%s]", bpmUUID.c_str());
  }

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
        capnp::byte *audioChunkStart = &audioChunk.front();
        size_t bytesLeft = audioChunk.size();
        size_t offset = 0;
        while (bytesLeft) {
          size_t sendSize = std::min(4096, static_cast<int>(bytesLeft));
          auto request = controllerServer.pushAudioDataRequest();
          auto builder = controllerServer.pushAudioDataRequest().initData();
          builder.setUuid(sessionUUID);
          kj::ArrayPtr<const capnp::byte> arrayPtr(audioChunkStart + offset,
                                                   sendSize);
          builder.setSegment(arrayPtr);
          request.setData(builder);

          //   logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
          // "Sending [%d] bytes... [%d ] bytes left", sendSize, bytesLeft);

          offset += sendSize;
          bytesLeft -= sendSize;

          auto promise = request.send();
          auto response = promise.wait(waitScope);
        }
      }
      audioQueue.clear();
    }

    // Get new BPM...
    {
      auto request = controllerServer.getBPMPipeLineDataRequest();
      request.setUuid(bpmUUID);
      auto promise = request.send();
      auto response = promise.wait(waitScope);
      auto bpmData = response.getResult();
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "BPM [%f] [%f%%]",
                      bpmData.getBpm(), bpmData.getConfidence());
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

void AudioProcessor::processAudio(const capnp::byte *musicData,
                                  const size_t musicDataLength) const noexcept {
  //   logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
  //   "Processing Chunk [%d] ", musicDataLength);
  {
    std::scoped_lock<std::mutex> lock(audioQueueMutex);
    audioQueue.push_back(
        std::vector<capnp::byte>(musicData, musicData + musicDataLength));
  }
  audioProcessorWakeup.notify_all();
}