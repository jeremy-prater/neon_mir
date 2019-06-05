#include "debuglogger.hpp"
#include "pulse_audio_stream.hpp"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unistd.h>

//////////////////////////////////////////////////////////////////////
//
// Audio processor definition
//

class AudioProcessor {
public:
  AudioProcessor();
  ~AudioProcessor();

  void processAudio(const void *data, const size_t size);

private:
  DebugLogger logger;
};

//////////////////////////////////////////////////////////////////////
//
// Audio processor implementation
//

AudioProcessor::AudioProcessor()
    : logger("AudioProcessor-", DebugLogger::DebugColor::COLOR_RED, false) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Created AudioProcessor");
}

AudioProcessor::~AudioProcessor() {}

void AudioProcessor::processAudio(const void *data, const size_t size) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Processing Chunk [%d]",
                  size);
}

//////////////////////////////////////////////////////////////////////
//
// Globals
//

NeonPulseInput paInput;
boost::asio::io_service io_service;
AudioProcessor audioProcessor;

int main() {
  paInput.Connect();

  auto dataConnection = paInput.newData.connect(
      boost::bind(&AudioProcessor::processAudio, &audioProcessor, _1, _2));

  paInput.CreateStream(2, 44100, PA_SAMPLE_S16LE);

  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
  signals.async_wait([&dataConnection](const boost::system::error_code &error,
                                       int signal_number) {
    dataConnection.disconnect();
    paInput.DestroyStream();
    paInput.Disconnect();
    io_service.stop();
  });

  io_service.run();
}