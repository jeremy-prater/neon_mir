#include "session-server.hpp"
#include "audio-session.hpp"
#include "boost/uuid/random_generator.hpp"
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
  capnp::EzRpcServer server(kj::heap<SessionServer::Handler>(this), "*", 5554);
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
  std::string name = context.getParams().getName();
  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                            "Received Create Session Request for [%s]",
                            name.c_str());

  boost::uuids::uuid uuid;
  std::shared_ptr<AudioSession> newSession;

  bool collision = false;
  {
    std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);

    do {
      uuid = boost::uuids::random_generator()();
      auto it = AudioSession::activeSessions.find(uuid);
      if (it != AudioSession::activeSessions.end()) {
        instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                  "activeSession UUID collision [%s]",
                                  boost::uuids::to_string(uuid).c_str());
        collision = true;
      }
    } while (collision);

    newSession = std::make_shared<AudioSession>(uuid);
    AudioSession::activeSessions[uuid] = newSession;
  }

  std::string uuidString = boost::uuids::to_string(uuid);
  if (name.find("Test-Client-") != std::string::npos)
    uuidString = name;
  context.getResults().setUuid(uuidString);

  return kj::READY_NOW;
}

kj::Promise<void>
SessionServer::Handler::releaseSession(ReleaseSessionContext context) {
  instance->logger.WriteLog(
      DebugLogger::DebugLevel::DEBUG_WARNING,
      "NOT IMPLEMENTED ==> Received Destroy Session Request for [%s]",
      context.getParams().getUuid().cStr());

  // TODO : implement
  return kj::READY_NOW;
}

kj::Promise<void> SessionServer::Handler::shutdown(ShutdownContext context) {
  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                            "Session Server shutdown!!");
  instance->running = false;
  return kj::READY_NOW;
}

kj::Promise<void> SessionServer::Handler::updateSessionConfig(
    UpdateSessionConfigContext context) {
  auto config = context.getParams().getConfig();

  std::string uuid = config.getUuid();
  {
    std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);

    auto it = AudioSession::activeSessions.find(
        boost::uuids::string_generator()(uuid));
    if (it == AudioSession::activeSessions.end()) {
      instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                                "Unknown UUID [%s]", uuid.c_str());
    } else {
      it->second->updateConfig(config.getSampleRate(), config.getChannels(),
                               config.getWidth(), config.getDuration());
    }
  }
  return kj::READY_NOW;
}

kj::Promise<void>
SessionServer::Handler::pushAudioData(PushAudioDataContext context) {
  auto input = context.getParams().getData();
  std::string uuid = input.getUuid();
  auto newAudio = input.getSegment();

  // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
  //                           "pushAudioData to [%s] [%d] bytes", uuid.c_str(),
  //                           newAudio.size());

  {
    std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);

    auto it = AudioSession::activeSessions.find(
        boost::uuids::string_generator()(uuid));
    if (it == AudioSession::activeSessions.end()) {
      instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                "Unknown Session UUID [%s]", uuid.c_str());
    } else {
      std::scoped_lock<std::mutex> lock(it->second->audioSinkMutex);
      // Transform data...??
      auto output = it->second->getAudioSink();

      // Optimize this later!!
      for (auto audioByte : newAudio)
        output->push_back(audioByte);

      // instance->logger.WriteLog(
      //     DebugLogger::DebugLevel::DEBUG_STATUS,
      //     "pushAudioData - circular buffer size [%s] [%d]", uuid.c_str(),
      //     output->size());
    }
  }

  return kj::READY_NOW;
}

kj::Promise<void>
SessionServer::Handler::releasePipeline(ReleasePipelineContext context) {
  std::string uuid = context.getParams().getUuid();

  {
    std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);

    auto it = AudioSession::activeSessions.find(
        boost::uuids::string_generator()(uuid));
    if (it == AudioSession::activeSessions.end()) {
      instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                "Unknown Pipeline UUID [%s]", uuid.c_str());
    } else {
      instance->logger.WriteLog(
          DebugLogger::DebugLevel::DEBUG_WARNING,
          "NOT IMPLEMENTED ==> Destroying Pipeline UUID [%s]", uuid.c_str());
    }
  }

  return kj::READY_NOW;
}

kj::Promise<void>
SessionServer::Handler::createBPMPipeLine(CreateBPMPipeLineContext context) {
  std::string audioSessionUUID = context.getParams().getUuid();

  boost::uuids::uuid essentiaSessionUUID;
  std::shared_ptr<NeonEssentiaSession> newSession;

  bool collision = false;
  {
    std::scoped_lock<std::mutex> lock(AudioSession::activePipelinesMutex);

    do {
      essentiaSessionUUID = boost::uuids::random_generator()();
      auto it = AudioSession::activePipelines.find(essentiaSessionUUID);
      if (it != AudioSession::activePipelines.end()) {
        instance->logger.WriteLog(
            DebugLogger::DebugLevel::DEBUG_WARNING,
            "activePipelines UUID collision [%s]",
            boost::uuids::to_string(essentiaSessionUUID).c_str());
        collision = true;
      }
    } while (collision);

    std::string uuidString = boost::uuids::to_string(essentiaSessionUUID);
    newSession =
        std::make_shared<NeonEssentiaSession>(audioSessionUUID, uuidString);
    AudioSession::activePipelines[essentiaSessionUUID] = newSession;
    context.getResults().setUuid(uuidString);
  }

  {
    std::scoped_lock<std::mutex> lock(AudioSession::activeSessionMutex);
    auto it = AudioSession::activeSessions.find(
        boost::uuids::string_generator()(audioSessionUUID));
    if (it == AudioSession::activeSessions.end()) {
      instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                "activeSession UUID not found [%s]",
                                audioSessionUUID.c_str());
    } else {
      auto audioSession = it->second;
      newSession->createBPMPipeline(
          audioSession->getSampleRate(), audioSession->getChannels(),
          audioSession->getWidth(), audioSession->getDuration());
    }
  }

  return kj::READY_NOW;
}

kj::Promise<void>
SessionServer::Handler::getBPMPipeLineData(GetBPMPipeLineDataContext context) {
  std::string bpmUUID = context.getParams().getUuid();
  {
    std::scoped_lock<std::mutex> lock(AudioSession::activePipelinesMutex);

    auto it = AudioSession::activePipelines.find(
        boost::uuids::string_generator()(bpmUUID));
    if (it == AudioSession::activePipelines.end()) {
      instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                "Unknown Pipeline UUID [%s]", bpmUUID.c_str());
    } else {
      essentia::Real bpm;
      essentia::Real confidence;
      it->second->runBPMPipeline(&bpm, &confidence);
      instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                      "Results : BPM [%f] Confidence [%f%%]", bpm, confidence);

      auto result = context.getResults().initResult();
      result.setBpm(bpm);
      result.setConfidence(bpm);
      context.getResults().setResult(result);
    }
  }

  return kj::READY_NOW;
}
