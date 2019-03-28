#pragma once

#include <boost/network/protocol/http/server.hpp>

#include "debuglogger.hpp"

class CoreHandler;

typedef boost::network::http::server<CoreHandler> CoreHandlerServer;

class CoreHandler {
public:
  CoreHandler();
  ~CoreHandler();

  void operator()(CoreHandlerServer::request const &request,
                  CoreHandlerServer::connection_ptr connection);
  void log(const CoreHandlerServer::string_type &message);

private:
  DebugLogger logger;
};
