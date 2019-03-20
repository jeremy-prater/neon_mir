#pragma once

#include "audio-session.hpp"
#include "core-handler.hpp"
#include "debuglogger.hpp"
#include "session-header.hpp"
#include <boost/uuid/uuid_io.hpp>
#include <pistache/endpoint.h>
#include "essentia.hpp"

using namespace Pistache;

class NeonMIR {
public:
  NeonMIR();
  ~NeonMIR();

  static NeonMIR *getInstance();

  void StartServer();
  void Shutdown();

  NeonEssentia neonEssentia;

private:
  DebugLogger logger;

  static NeonMIR *instance;

  std::shared_ptr<Http::Endpoint> server;

  std::shared_ptr<CoreHandler> coreHandler;

};