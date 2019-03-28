#pragma once

#include "core-handler.hpp"

#include "audio-session.hpp"
#include "debuglogger.hpp"
#include "essentia.hpp"
#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/uuid/uuid_io.hpp>

class NeonMIR {
public:
  NeonMIR();
  ~NeonMIR();

  static NeonMIR *getInstance();

  void StartServer();
  void Shutdown();

  NeonEssentia neonEssentia;

private:
  boost::asio::io_service io_service;
  DebugLogger logger;

  std::thread httpServerThread;
  std::shared_ptr<CoreHandlerServer> localServer;

  static NeonMIR *instance;
};