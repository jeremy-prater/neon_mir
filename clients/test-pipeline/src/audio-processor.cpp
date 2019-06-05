#include "audio-processor.hpp"

//////////////////////////////////////////////////////////////////////
//
// Audio processor implementation
//

AudioProcessor::AudioProcessor()
    : client("localhost:5554"),
      controllerServer(client.getMain<neon::session::Controller>()),
      waitScope(client.getWaitScope()), handle("test-pipeline"), sessionUUID(),
      defaultSampleRate(44100), defaultChannels(2), defaultWidth(16),
      defaultDurationMs(10 * 1000),
      logger("AudioProcessor-", DebugLogger::DebugColor::COLOR_RED, false) {
  {
    auto request = controllerServer.createSessionRequest();
    request.setName(handle);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
    sessionUUID = response.getUuid();
  }

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Created AudioProcessor ==> Essentia UUID [%s]",
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
}

AudioProcessor::~AudioProcessor() {}

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
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Processing Chunk [%d]",
                  musicDataLength);

  // Write music sample... in 1024 byte chunks?
  size_t bytesLeft = musicDataLength;
  size_t offset = 0;
  while (bytesLeft) {
    size_t sendSize = std::min(4096, static_cast<int>(bytesLeft));
    auto request = controllerServer.pushAudioDataRequest();
    auto builder = controllerServer.pushAudioDataRequest().initData();
    builder.setUuid(sessionUUID);
    kj::ArrayPtr<const capnp::byte> arrayPtr(musicData + offset, sendSize);
    builder.setSegment(arrayPtr);
    request.setData(builder);

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Sending [%d] bytes... [%d ] bytes left", sendSize,
                    bytesLeft);

    offset += sendSize;
    bytesLeft -= sendSize;

    auto promise = request.send();
    auto response = promise.wait(waitScope);
  }
}