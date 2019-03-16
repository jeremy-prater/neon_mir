#pragma once

#include "debuglogger.hpp"
#include <pistache/endpoint.h>

using namespace Pistache;

class CoreHandler : public Http::Handler {
  HTTP_PROTOTYPE(CoreHandler)
public:
  CoreHandler();
  ~CoreHandler();

  void onRequest(const Http::Request &request,
                 Http::ResponseWriter response) override;

private:
  DebugLogger logger;
};
