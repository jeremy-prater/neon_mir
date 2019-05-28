#pragma once

#include "audio-session.hpp"
#include "debuglogger.hpp"
#include "essentia.hpp"
#include "session-server.hpp"
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

  SessionServer localServer;

  static NeonMIR *instance;
};