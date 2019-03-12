#include "core-handler.hpp"

CoreHandler::CoreHandler() {}
CoreHandler::~CoreHandler() {}

void CoreHandler::onRequest(const Http::Request &request,
                            Http::ResponseWriter response) override {

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Processing Request [%s][%d]", request.resource().c_str(),
                  request.method());

  switch (request.method()) {
  case Http::Method::Post:
    if (request.resource() == "/shutdown") {
      server->shutdown();
      return;
    } else if (request.resource() == "/newSession") {

      AudioSession newSession(request.body().c_str(), request.body().length());

      response.headers().add<NeonSessionHeader>(
          boost::uuids::to_string(newSession.uuid));
      response.send(Http::Code::Ok, "");
    }
    break;

  default:
    response.send(Http::Code::Not_Found, "Invalid Request");
  }
}