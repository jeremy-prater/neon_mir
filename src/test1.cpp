#include "debuglogger.hpp"
#include <pistache/endpoint.h>

using namespace Pistache;

DebugLogger logger("NEON-MIR", DebugLogger::DebugColor::COLOR_WHITE, true);

struct HelloHandler : public Http::Handler {
  HTTP_PROTOTYPE(HelloHandler)
  void onRequest(const Http::Request &request,
                 Http::ResponseWriter writer) override {

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Starting Server!");
    writer.send(Http::Code::Ok, "Hello, World!");
  }
};

int main() {
  const uint8_t threads = 8;

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Start up");
  Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(8351));

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Using [%d] threads",
                  threads);
  auto opts = Http::Endpoint::options().threads(threads);
  Http::Endpoint server(addr);
  server.init(opts);

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Adding HelloHandler");
  server.setHandler(std::make_shared<HelloHandler>());

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Starting Server!");
  server.serve();
}