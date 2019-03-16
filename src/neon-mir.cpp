#include "neon-mir.hpp"

using namespace Pistache;

NeonMIR *NeonMIR::instance = nullptr;

NeonMIR::NeonMIR()
    : logger("NEON-MIR", DebugLogger::DebugColor::COLOR_WHITE, true) {

  const uint8_t threads = hardware_concurrency();

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Start up");
  Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(8351));

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Using [%d] threads",
                  threads);
  auto server_options =
      Pistache::Http::Endpoint::options().threads(threads).flags(
          Tcp::Options::InstallSignalHandler);
  server = std::make_shared<Http::Endpoint>(addr);
  server->init(server_options);

  Http::Header::Registry::instance().registerHeader<NeonSessionHeader>();

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Adding CoreHandler");
  coreHandler = std::make_shared<CoreHandler>();
  server->setHandler(coreHandler);
}

NeonMIR::~NeonMIR() {}

NeonMIR *NeonMIR::getInstance() {
  static std::mutex instanceMutex;

  std::scoped_lock<std::mutex> lock(instanceMutex);
  if (!instance)
    instance = new NeonMIR();
  return instance;
}

void NeonMIR::StartServer() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Starting Server!");
  server->serve();
}

void NeonMIR::Shutdown() { server->shutdown(); }

int main(int argc, char **argv) { NeonMIR::getInstance()->StartServer(); }
