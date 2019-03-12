#include "neon-mir.hpp"

using namespace Pistache;

NeonMIR::NeonMIR()
    : logger("NEON-MIR", DebugLogger::DebugColor::COLOR_WHITE, true) {

  const uint8_t threads = 8;

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Start up");
  Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(8351));

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Using [%d] threads",
                  threads);
  server_options = Http::Endpoint::options().threads(threads);
  // .flags(Tcp::Options::InstallSignalHandler);
  server = std::make_shared<Http::Endpoint>(addr);
  server->init(opts);

  Http::Header::Registry::instance().registerHeader<NeonSessionHeader>();

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Adding HelloHandler");
  server->setHandler(std::make_shared<HelloHandler>());

  Routes::Get(router, "/users/:sessionid",
              Routes::bind(&AudioSession::getSessionID, this));

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Starting Server!");
  server->serve();
}

NeonMIR::~NeonMIR() {}