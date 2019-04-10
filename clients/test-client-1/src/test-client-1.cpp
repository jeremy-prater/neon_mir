//
//  Hello World client in C++
//  Connects REQ socket to tcp://localhost:5555
//  Sends "Hello" to server, expects "World" back
//
#include "capnp/serialize.h"
#include "neon.session.capnp.h"
#include <ZMQOutputStream.hpp>
#include <iostream>
#include <string>
#include <zmq.hpp>

int main() {
  //  Prepare our context and socket
  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REQ);

  std::cout << "Connecting to hello world server..." << std::endl;
  socket.connect("tcp://localhost:5555");

  capnp::MallocMessageBuilder message;

  auto sessionEvent = message.initRoot<neon::session::SessionEvent>();

  std::string handle = "Test-Client-1";
  sessionEvent.setCommand(
      neon::session::SessionEvent::Command::RELEASE_SESSION);
  sessionEvent.setName(handle);
  sessionEvent.setUuid("12345");

  ZMQOutputStream outStream;
  capnp::writeMessage(outStream, message);
  zmq::message_t request(outStream.data(), outStream.size(),
                         &ZMQOutputStream::release, static_cast<void *>(&outStream));

  std::cout << "Sending " << request.size() << " bytes" << std::endl;
  std::cout << "Sending CREATE_SESSION [" << handle << "]" << std::endl;

  socket.send(request);

  //  Get the reply.
  zmq::message_t reply;
  socket.recv(&reply);
  std::cout << "Received UUID ["
            << "12345"
            << "]" << std::endl;

  return 0;
}
