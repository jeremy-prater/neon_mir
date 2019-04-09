#include "neon-mir.hpp"

NeonMIR *NeonMIR::instance = nullptr;

NeonMIR::NeonMIR()
    : logger("NEON-MIR", DebugLogger::DebugColor::COLOR_WHITE, true) {}

NeonMIR::~NeonMIR() {}

NeonMIR *NeonMIR::getInstance() {
  static std::mutex instanceMutex;

  std::scoped_lock<std::mutex> lock(instanceMutex);
  if (!instance)
    instance = new NeonMIR();
  return instance;
}

void NeonMIR::StartServer() {
  boost::asio::high_resolution_timer timer{io_service};

  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
  signals.async_wait(
      [&](const boost::system::error_code &error, int signal_number) {
        (void)error;
        (void)signal_number;

        logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                        "Received signal, shutting down");
        io_service.stop();
      });

  std::function<void()> timerTick;
  timerTick = [&] {
    timer.expires_after(std::chrono::milliseconds(250));

    timer.async_wait([&](std::error_code ec) {
      if (!ec)
        timerTick();
    });
  };

  //   timerTick();

  io_service.run();
}

void NeonMIR::Shutdown() { io_service.stop(); }

int main(int argc, char **argv) { NeonMIR::getInstance()->StartServer(); }
