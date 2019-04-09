#pragma once

#include "debuglogger.hpp"
#include <thread>
#include <zmq.hpp>

class SessionServer {
public:

  SessionServer();
  ~SessionServer();

private:
  void server();

  DebugLogger logger;
  bool shutdown;
  std::thread sessionThread;

  zmq::context_t zmqContext;
  zmq::socket_t zmqSocket;
};
