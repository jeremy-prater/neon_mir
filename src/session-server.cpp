#include "session-server.hpp"
#include "audio-session.hpp"
#include "boost/uuid/string_generator.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "capnp/ez-rpc.h"
#include "capnp/serialize.h"
#include "neon-mir.hpp"
#include "neon.session.capnp.h"

SessionServer::SessionServer()
    : running(true),
      logger("SessionServer", DebugLogger::DebugColor::COLOR_MAGENTA, true),
      zmqContext(std::thread::hardware_concurrency()),
      zmqSocket(zmqContext, ZMQ_REP) {
  sessionThread = std::thread(&SessionServer::server, this);
}

SessionServer::~SessionServer() {
  running = false;
  if (sessionThread.joinable())
    sessionThread.join();
}

void SessionServer::server() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Starting SessionServer");
  zmqSocket.bind("tcp://*:5555");

  // Run forever, accepting connections and handling requests.
  capnp::EzRpcServer server(kj::heap<SessionServer::Handler>(this), "*",
                            5554);
  auto &waitScope = server.getWaitScope();
  kj::NEVER_DONE.wait(waitScope);

  while (running) {
    zmq::message_t request;

    //  Wait for next request from client
    zmqSocket.recv(&request);

    // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
    //                 "Received Request [%d] bytes", request.size());

    // capnp::UnalignedFlatArrayMessageReader reader(kj::arrayPtr(
    //     static_cast<const capnp::word *>(request.data()), request.size()));

    // neon::session::SessionEvent::Reader sessionEvent =
    //     reader.getRoot<neon::session::SessionEvent>();

    // switch (sessionEvent.getCommand()) {
    // case neon::session::SessionEvent::Command::CREATE_SESSION: {
    // } break;
    // case neon::session::SessionEvent::Command::RELEASE_SESSION: {
    //   logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
    //                   "Received Release Session Request for [%s]",
    //                   sessionEvent.getName().cStr());
    //   zmq::message_t reply;
    //   zmqSocket.send(reply);

    // } break;
    // case neon::session::SessionEvent::Command::SHUTDOWN: {
    //   logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Shutdown!");
    //   zmq::message_t reply;
    //   zmqSocket.send(reply);
    // } break;
    // }
  }

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Stopped SessionServer");
}

SessionServer::Handler::Handler(SessionServer *server) : instance(server) {}
SessionServer::Handler::~Handler() {}

kj::Promise<void>
SessionServer::Handler::createSession(CreateSessionContext context) {
  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                            "Received Create Session Request for [%s]",
                            context.getParams().getName().cStr());

  std::shared_ptr<AudioSession> newSession;
  bool collision = false;

  {
    std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);
    do {
      newSession = std::make_shared<AudioSession>();
      auto it = AudioSession::activeSessions.find(newSession->uuid);
      if (it != AudioSession::activeSessions.end()) {
        instance->logger.WriteLog(
            DebugLogger::DebugLevel::DEBUG_WARNING,
            "activeSession UUID collision [%s]",
            boost::uuids::to_string(newSession->uuid).c_str());
        collision = true;
      }
    } while (collision);
    AudioSession::activeSessions[newSession->uuid] = newSession;
  }
  std::string uuidString = boost::uuids::to_string(newSession->uuid);

  // For testing!!
  // context.getResults().setUuid(context.getParams().getName());
  context.getResults().setUuid(uuidString);

  return kj::READY_NOW;
}

kj::Promise<void>
SessionServer::Handler::releaseSession(ReleaseSessionContext context) {
  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                            "Received Destroy Session Request for [%s]",
                            context.getParams().getUuid().cStr());
  return kj::READY_NOW;
}

kj::Promise<void> SessionServer::Handler::shutdown(ShutdownContext context) {
  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                            "Session Server shutdown!!");
  return kj::READY_NOW;

  instance->running = false;
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
