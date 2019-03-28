#include "neon-mir.hpp"

NeonMIR *NeonMIR::instance = nullptr;

NeonMIR::NeonMIR()
    : logger("NEON-MIR", DebugLogger::DebugColor::COLOR_WHITE, true) {}

NeonMIR::~NeonMIR() {
  if (httpServerThread.joinable()) {
    localServer->stop();
    httpServerThread.join();
  }
}

NeonMIR *NeonMIR::getInstance() {
  static std::mutex instanceMutex;

  std::scoped_lock<std::mutex> lock(instanceMutex);
  if (!instance)
    instance = new NeonMIR();
  return instance;
}

void NeonMIR::StartServer() {
  httpServerThread = std::thread([this] {
    try {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                      "Starting HTTP Server!");
      CoreHandler handler;
      CoreHandlerServer::options options(handler);
      localServer = std::make_shared<CoreHandlerServer>(
          options
              .thread_pool(std::make_shared<boost::network::utils::thread_pool>(
                  std::thread::hardware_concurrency()))
              .address("0.0.0.0")
              .port("8351"));
      localServer->run();
    } catch (std::exception &e) {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                      "HTTP Server Error : %s", e.what());
    }
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                    "Stopped HTTP Server!");
  });

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
