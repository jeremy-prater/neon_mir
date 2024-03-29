#pragma once

#include "debuglogger.hpp"
#include "neon.session.capnp.h"
#include <thread>
#include <zmq.hpp>

class SessionServer {
public:
  SessionServer();
  ~SessionServer();

  class Handler final : public neon::session::Controller::Server {
  public:
    Handler(SessionServer *server);
    virtual ~Handler();

    virtual ::kj::Promise<void>
    createSession(CreateSessionContext context) override;
    virtual ::kj::Promise<void>
    releaseSession(ReleaseSessionContext context) override;
    virtual ::kj::Promise<void> shutdown(ShutdownContext context) override;
    virtual ::kj::Promise<void>
    updateSessionConfig(UpdateSessionConfigContext context) override;
    virtual ::kj::Promise<void>
    pushAudioData(PushAudioDataContext context) override;

    virtual ::kj::Promise<void>
    releasePipeline(ReleasePipelineContext context) override;

    virtual ::kj::Promise<void>
    createBPMPipeLine(CreateBPMPipeLineContext context) override;
    virtual ::kj::Promise<void>
    getBPMPipeLineData(GetBPMPipeLineDataContext context) override;

    virtual ::kj::Promise<void>
    createSpectrumPipe(CreateSpectrumPipeContext context) override;
    virtual ::kj::Promise<void>
    getSpectrumData(GetSpectrumDataContext context) override;

  private:
    SessionServer *instance;
  };

private:
  void server();

  DebugLogger logger;
  bool running;
  std::thread sessionThread;

  zmq::context_t zmqContext;
  zmq::socket_t zmqSocket;
};
