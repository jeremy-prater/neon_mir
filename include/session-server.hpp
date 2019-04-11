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
