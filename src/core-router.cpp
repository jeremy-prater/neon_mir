#include "core-router.hpp"

CoreRouter::CoreRouter() {
  Rest::Route::Get(router, "/users/:sessionid",
                   Rest::Routes::bind(&CoreRouter::getSessionID, this));
}

CoreRouter::~CoreRouter() {}

void CoreRouter::getSessionID(const Rest::Request &request,
                              Http::ResponseWriter response) {}
