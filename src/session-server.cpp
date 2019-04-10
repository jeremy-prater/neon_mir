#include "session-server.hpp"
#include "audio-session.hpp"
#include "boost/uuid/string_generator.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "capnp/serialize.h"
#include "neon-mir.hpp"
#include "neon.session.capnp.h"

SessionServer::SessionServer()
    : shutdown(false),
      logger("SessionServer", DebugLogger::DebugColor::COLOR_MAGENTA, true),
      zmqContext(std::thread::hardware_concurrency()),
      zmqSocket(zmqContext, ZMQ_REP) {
  sessionThread = std::thread(&SessionServer::server, this);
}

SessionServer::~SessionServer() {
  shutdown = true;
  if (sessionThread.joinable())
    sessionThread.join();
}

void SessionServer::server() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Starting SessionServer");
  zmqSocket.bind("tcp://*:5555");

  while (!shutdown) {
    zmq::message_t request;

    //  Wait for next request from client
    zmqSocket.recv(&request);

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                    "Received Request [%d] bytes", request.size());

    capnp::UnalignedFlatArrayMessageReader reader(kj::arrayPtr(
        static_cast<const capnp::word *>(request.data()), request.size()));

    neon::session::SessionEvent::Reader sessionEvent =
        reader.getRoot<neon::session::SessionEvent>();

    switch (sessionEvent.getCommand()) {
    case neon::session::SessionEvent::Command::CREATE_SESSION: {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                      "Received Create Session Request for [%s]",
                      sessionEvent.getName().cStr());
    } break;
    case neon::session::SessionEvent::Command::RELEASE_SESSION: {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                      "Received Release Session Request for [%s]",
                      sessionEvent.getName().cStr());
    } break;
    case neon::session::SessionEvent::Command::SHUTDOWN: {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Shutdown!");
    } break;
    }

    //       std::shared_ptr<AudioSession> newSession;
    //       bool collision = false;
    //       std::string payload;
    //       {
    //         std::scoped_lock<std::mutex>
    //         lock(AudioSession::activeSessionMutex); do {
    //           newSession =
    //               std::make_shared<AudioSession>(payload.c_str(),
    //               payload.length());
    //           auto it = AudioSession::activeSessions.find(newSession->uuid);
    //           if (it != AudioSession::activeSessions.end()) {
    //             logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
    //                             "activeSession UUID collision [%s]",
    //                             boost::uuids::to_string(newSession->uuid).c_str());
    //             collision = true;
    //           }
    //         } while (collision);
    //         AudioSession::activeSessions[newSession->uuid] = newSession;
    //       }
    //       headers["SessionID"] = boost::uuids::to_string(newSession->uuid);

    //  Send reply back to client
    zmq::message_t reply(5);
    memcpy(reply.data(), "World", 5);
    zmqSocket.send(reply);
  }
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Stopped SessionServer");
}

// void SessionServer::operator()(SessionServerServer::request const &request,
//                              SessionServerServer::connection_ptr connection)
//                              {
//   logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "HTTP Request : %s
//   %s",
//                   request.method.c_str(), request.destination.c_str());

//   bool failure = true;
//   std::string responseBody;
//   std::map<std::string, std::string> headers = {{"Content-Type",
//   "text/json"}};

//   if (request.method == "POST") {
//     if (request.destination == "/shutdown") {
//       NeonMIR::getInstance()->Shutdown();
//       return;
//     } else if (request.destination == "/newSession") {
//       std::shared_ptr<AudioSession> newSession;
//       bool collision = false;

//       std::string payload;
//       connection->read(
//           [&payload](SessionServerServer::connection::input_range chunk,
//                      std::error_code ec, std::size_t bytes_transferred,
//                      SessionServerServer::connection_ptr connection) {
//             for (auto &c : chunk)
//               payload += c;
//           });

//       usleep(500 * 1000);

//       {
//         std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);
//         do {
//           newSession =
//               std::make_shared<AudioSession>(payload.c_str(),
//               payload.length());
//           auto it = AudioSession::activeSessions.find(newSession->uuid);
//           if (it != AudioSession::activeSessions.end()) {
//             logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
//                             "activeSession UUID collision [%s]",
//                             boost::uuids::to_string(newSession->uuid).c_str());
//             collision = true;
//           }
//         } while (collision);
//         AudioSession::activeSessions[newSession->uuid] = newSession;
//       }

//       connection->set_status(SessionServerServer::connection::ok);
//       failure = false;
//       headers["SessionID"] = boost::uuids::to_string(newSession->uuid);

//     } else if (request.destination == "/releaseSession") {
//       std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);
//       failure = false;
//       //   auto headers = request.headers;
//       //   std::string uuidHeader;
//       //   auto neonSessionHdr = request.headers;
//       //   neonSessionHdr.get<NeonSessionHeader>()->parse(uuidHeader);

//       //   boost::uuids::string_generator uuidGenerator;
//       //   boost::uuids::uuid targetUUID = uuidGenerator(uuidHeader);
//       //   auto it = AudioSession::activeSessions.find(targetUUID);
//       //   if (it != AudioSession::activeSessions.end()) {
//       //     AudioSession::activeSessions.erase(targetUUID);
//       //     response.send(Http::Code::Ok, "");
//       //   } else {
//       //     logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
//       //                     "Unable to find audio session [%s] to delete",
//       //                     uuidHeader.c_str());
//       //     response.send(Http::Code::Not_Found, "");
//       //   }
//     }
//   }

//   if (failure) {
//     connection->set_status(SessionServerServer::connection::bad_request);
//     responseBody = "Invalid Request";
//   }

//   headers["Content-Length"] = std::to_string(responseBody.length());
//   connection->set_headers(headers);
//   connection->write(responseBody);
// }
