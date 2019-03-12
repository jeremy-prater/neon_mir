#include "audio-session.hpp"
#include "debuglogger.hpp"
#include "session-header.hpp"
#include <boost/uuid/uuid_io.hpp>
#include <pistache/endpoint.h>
#include <pistache/router.h>

using namespace Pistache;

class NeonMIR {
public:
  NeonMIR();
  ~NeonMIR();

private:

  DebugLogger logger;

  std::shared_ptr<Http::Endpoint> server;

  Http::Endpoint::Options server_options;
  Http::Router router
};