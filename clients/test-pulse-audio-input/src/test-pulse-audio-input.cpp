#include "debuglogger.hpp"
#include "pulse_audio_stream.hpp"
#include <boost/asio.hpp>
#include <unistd.h>

NeonPulseInput paInput;
boost::asio::io_service io_service;

int main() {
  paInput.PAConnect();
  paInput.Connect();

  // auto sources = paInput.getSources();

  paInput.CreateStream(0);
  paInput.StartStream();

  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
  signals.async_wait(
      [&](const boost::system::error_code &error, int signal_number) {
        paInput.StopStream();
        paInput.DestroyStream();
        paInput.Disconnect();
        io_service.stop();
      });

  io_service.run();
}