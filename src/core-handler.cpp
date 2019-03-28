#include "core-handler.hpp"
#include "audio-session.hpp"
#include "boost/uuid/string_generator.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "neon-mir.hpp"

CoreHandler::CoreHandler()
    : logger("CoreHandler", DebugLogger::DebugColor::COLOR_MAGENTA, true) {}

CoreHandler::~CoreHandler() {}

void CoreHandler::operator()(CoreHandlerServer::request const &request,
                             CoreHandlerServer::connection_ptr connection) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "HTTP Request : %s %s",
                  request.method.c_str(), request.destination.c_str());

  bool failure = true;
  std::string responseBody;
  std::map<std::string, std::string> headers = {{"Content-Type", "text/json"}};

  if (request.method == "POST") {
    if (request.destination == "/shutdown") {
      NeonMIR::getInstance()->Shutdown();
      return;
    } else if (request.destination == "/newSession") {
      std::shared_ptr<AudioSession> newSession;
      bool collision = false;

      CoreHandlerServer::connection::read_callback_function callbackFunc =
          [this](CoreHandlerServer::connection::input_range input,
                 boost::system::error_code ec, std::size_t bytes_transferred,
                 CoreHandlerServer::connection_ptr connection) {
            logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                            "Data size [%d]", bytes_transferred);
          };

      connection->read(callbackFunc);

      {
        std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);
        do {
          newSession = std::make_shared<AudioSession>(request.body.c_str(),
                                                      request.body.length());
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

      connection->set_status(CoreHandlerServer::connection::ok);
      failure = false;
      headers["SessionID"] = boost::uuids::to_string(newSession->uuid);

    } else if (request.destination == "/releaseSession") {
      std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);
      failure = false;
      //   auto headers = request.headers;
      //   std::string uuidHeader;
      //   auto neonSessionHdr = request.headers;
      //   neonSessionHdr.get<NeonSessionHeader>()->parse(uuidHeader);

      //   boost::uuids::string_generator uuidGenerator;
      //   boost::uuids::uuid targetUUID = uuidGenerator(uuidHeader);
      //   auto it = AudioSession::activeSessions.find(targetUUID);
      //   if (it != AudioSession::activeSessions.end()) {
      //     AudioSession::activeSessions.erase(targetUUID);
      //     response.send(Http::Code::Ok, "");
      //   } else {
      //     logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
      //                     "Unable to find audio session [%s] to delete",
      //                     uuidHeader.c_str());
      //     response.send(Http::Code::Not_Found, "");
      //   }
    }
  }

  if (failure) {
    connection->set_status(CoreHandlerServer::connection::bad_request);
    responseBody = "Invalid Request";
  }

  headers["Content-Length"] = std::to_string(responseBody.length());
  connection->set_headers(headers);
  connection->write(responseBody);
}

void CoreHandler::log(const CoreHandlerServer::string_type &message) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                  "CoreHandler Error : %s", message.c_str());
}
