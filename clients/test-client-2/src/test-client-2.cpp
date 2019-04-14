#include "capnp/ez-rpc.h"
#include "capnp/serialize.h"
#include "neon.session.capnp.h"
#include <ZMQOutputStream.hpp>
#include <fstream>
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

  // Create Session
  {
    auto request = controllerServer.createSessionRequest();
    request.setName(handle);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
    sessionUUID = response.getUuid();
  }

  // Set Session config
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

  char *musicData;
  uint32_t musicDataLength;

  // Load music sample...
  {
    std::ifstream audioSource(
        "/home/prater/src/neon_mir/samples/midnight-drive-clip-10s.wav",
        std::ios::in | std::ios::binary);
    audioSource.seekg(0, std::ios::end);
    musicDataLength = audioSource.tellg();
    audioSource.seekg(0, std::ios::beg);
    musicData = static_cast<char *>(malloc(musicDataLength));
    audioSource.read(musicData, musicDataLength);
    audioSource.close();
  }

  // Write music sample...
  {
    auto request = controllerServer.pushAudioDataRequest();
    // Copy/Move data from musicData into the server.
    // ::kj::arrayPtr<const char *> arrayPtr(static_cast<const char *>(musicData),
    //                                       static_cast<size_t>(musicDataLength));
    // neon::session::SessionAudioPacket::Reader value;
    // request.setData(value);
    // request.send();
  }
}
