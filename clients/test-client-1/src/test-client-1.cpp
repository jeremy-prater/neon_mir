//
//  Hello World client in C++
//  Connects REQ socket to tcp://localhost:5555
//  Sends "Hello" to server, expects "World" back
//
#include "capnp/serialize.h"
#include "neon.session.capnp.h"
#include <iostream>
#include <string>
#include <zmq.hpp>

int main() {
  //  Prepare our context and socket
  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REQ);

  std::cout << "Connecting to hello world server..." << std::endl;
  socket.connect("tcp://localhost:5555");

  //  Do 10 requests, waiting each time for a response
  for (int request_nbr = 0; request_nbr != 10; request_nbr++) {

    capnp::MallocMessageBuilder message;

    SessionEvent::Builder sessionEvent = message.initRoot<SessionEvent>();

    sessionEvent.setCommand(SessionEvent::Command::CREATE_SESSION);
    sessionEvent.setName("Test-Client-1");

    auto output = message.getSegmentsForOutput();

    std::cout << "Sending " << output.size() << " bytes" << std::endl;

    zmq::message_t request(output.size());
    memcpy(request.data(), output.begin(), output.size());
    std::cout << "Sending Hello " << request_nbr << "..." << std::endl;
    socket.send(request);

    //  Get the reply.
    zmq::message_t reply;
    socket.recv(&reply);
    std::cout << "Received World " << request_nbr << std::endl;
  }
  return 0;
}
