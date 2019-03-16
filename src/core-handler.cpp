#include "core-handler.hpp"
#include "audio-session.hpp"
#include "boost/uuid/string_generator.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "neon-mir.hpp"
#include "session-header.hpp"

CoreHandler::CoreHandler()
    : logger("CoreHandler", DebugLogger::DebugColor::COLOR_WHITE, true) {}
CoreHandler::~CoreHandler() {}

void CoreHandler::onRequest(const Http::Request &request,
                            Http::ResponseWriter response) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Processing Request [%s][%d]", request.resource().c_str(),
                  request.method());

  switch (request.method()) {
  case Http::Method::Post:
    if (request.resource() == "/shutdown") {
      NeonMIR::getInstance()->Shutdown();
      return;
    } else if (request.resource() == "/newSession") {
      std::shared_ptr<AudioSession> newSession;
      bool collision = false;

      {
        std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);
        do {
          newSession = std::make_shared<AudioSession>(request.body().c_str(),
                                                      request.body().length());
          auto it = AudioSession::activeSessions.find(newSession->uuid);
          if (it != AudioSession::activeSessions.end()) {
            logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                            "activeSession UUID collision [%s]",
                            boost::uuids::to_string(newSession->uuid).c_str());
            collision = true;
          }
        } while (collision);
        AudioSession::activeSessions[newSession->uuid] = newSession;
      }

      response.headers().add<NeonSessionHeader>(
          boost::uuids::to_string(newSession->uuid));
      response.send(Http::Code::Ok, "");
    } else if (request.resource() == "/releaseSession") {
      std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);
      auto headers = request.headers();
      std::string uuidHeader;
      auto neonSessionHdr = request.headers();
      neonSessionHdr.get<NeonSessionHeader>()->parse(uuidHeader);

      boost::uuids::string_generator uuidGenerator;
      boost::uuids::uuid targetUUID = uuidGenerator(uuidHeader);
      auto it = AudioSession::activeSessions.find(targetUUID);
      if (it != AudioSession::activeSessions.end()) {
        AudioSession::activeSessions.erase(targetUUID);
        response.send(Http::Code::Ok, "");
      } else {
        logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                        "Unable to find audio session [%s] to delete",
                        uuidHeader.c_str());
        response.send(Http::Code::Not_Found, "");
      }
    }
    break;

  default:
    response.send(Http::Code::Not_Found, "Invalid Request");
  }
}