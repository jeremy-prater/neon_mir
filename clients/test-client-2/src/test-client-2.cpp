#include "capnp/ez-rpc.h"
#include "capnp/serialize.h"
#include "neon.session.capnp.h"
#include <fstream>
#include <iostream>
#include <kj/array.h>
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

  const std::string filename =
      "/home/prater/src/neon_mir/samples/midnight-drive-clip-10s.wav";
  // Create Session
  std::cout << "Creating Session" << std::endl;
  {
    auto request = controllerServer.createSessionRequest();
    request.setName(handle);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
    sessionUUID = response.getUuid();
  }

  std::cout << "I am [" << sessionUUID << "]" << std::endl;

  uint32_t defaultSampleRate = 44100;
  uint8_t defaultChannels = 2;
  uint8_t defaultWidth = 16;

  uint32_t defaultDurationMs = 10 * 1000;

  // Set Session config
  {
    auto request = controllerServer.updateSessionConfigRequest();
    auto config = controllerServer.updateSessionConfigRequest().initConfig();
    config.setUuid(sessionUUID);
    config.setSampleRate(defaultSampleRate);
    config.setChannels(defaultChannels);
    config.setWidth(defaultWidth);
    config.setDuration(defaultDurationMs);
    request.setConfig(config);
    auto promise = request.send();
    auto response = promise.wait(waitScope);
  }

  capnp::byte *musicData;
  size_t musicDataLength;

  std::cout << "Loading : " << filename << std::endl;
  // Load music sample...
  {
    std::ifstream audioSource(filename, std::ios::in | std::ios::binary);
    audioSource.seekg(0, std::ios::end);
    musicDataLength = audioSource.tellg();
    audioSource.seekg(0, std::ios::beg);
    musicData = static_cast<capnp::byte *>(malloc(musicDataLength));
    audioSource.read(reinterpret_cast<char *>(musicData), musicDataLength);
    audioSource.close();
  }

  std::cout << "Loaded [" << musicDataLength << "] bytes!" << std::endl;

  // Write music sample... in 1024 byte chunks?
  size_t bytesLeft = musicDataLength;
  size_t offset = 0;
  while (bytesLeft) {
    size_t sendSize = std::min(4096, static_cast<int>(bytesLeft));
    auto request = controllerServer.pushAudioDataRequest();
    auto builder = controllerServer.pushAudioDataRequest().initData();
    builder.setUuid(sessionUUID);
    kj::ArrayPtr<capnp::byte> arrayPtr(musicData + offset, sendSize);
    builder.setSegment(arrayPtr);
    request.setData(builder);

    std::cout << "Sending [" << sendSize << "] bytes! [" << bytesLeft
              << "] bytes left!" << std::endl;

    offset += sendSize;
    bytesLeft -= sendSize;

    auto promise = request.send();
    auto response = promise.wait(waitScope);
  }
}
