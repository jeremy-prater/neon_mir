//
//  Hello World client in C++
//  Connects REQ socket to tcp://localhost:5555
//  Sends "Hello" to server, expects "World" back
//
#include "capnp/ez-rpc.h"
#include "capnp/serialize.h"
#include "neon.session.capnp.h"
#include <ZMQOutputStream.hpp>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <zmq.hpp>

int main() {
  capnp::EzRpcClient client("localhost:5554");
  neon::session::Controller::Client controllerServer =
      client.getMain<neon::session::Controller>();
  auto &waitScope = client.getWaitScope();
  std::string handle = "Test-Client-2 : Audio Stream Test";
  std::string sessionUUID;

  {
    auto request = controllerServer.createSessionRequest();
    request.setName(handle);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
    sessionUUID = response.getUuid();
  }

  {
    auto request = controllerServer.updateSessionConfigRequest();
    auto config = controllerServer.updateSessionConfigRequest().initConfig();
    config.setUuid(sessionUUID);
    config.setSampleRate(44100);
    config.setChannels(2);
    config.setWidth(16);
    config.setDuration(10 * 1000);
    request.setConfig(config);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
  }
}
