#include "audio-processor.hpp"
#include "debuglogger.hpp"
#include "pulse_audio_stream.hpp"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unistd.h>

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

  paInput.CreateStream(audioProcessor.getChannels(),
                       audioProcessor.getSampleRate(), PA_SAMPLE_FLOAT32LE);

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