//
//  Hello World client in C++
//  Connects REQ socket to tcp://localhost:5555
//  Sends "Hello" to server, expects "World" back
//
#include "capnp/serialize.h"
#include "neon.session.capnp.h"
#include <ZMQOutputStream.hpp>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <zmq.hpp>

#define THREAD_COUNT 50
#define ITERATIONS 200

std::mutex countMutex;
uint32_t count;

class MismatchedReply : public std::exception {
  virtual const char *what() const throw() { return "MismatchedReply!!"; }
};

int main() {
  count = 0;
  std::thread threadPool[THREAD_COUNT];

  for (int tIC = 0; tIC < ITERATIONS; tIC++) {
    for (int tID = 0; tID < THREAD_COUNT; tID++) {
      threadPool[tID] = std::thread([tID] {
        zmq::context_t context(1);
        zmq::socket_t socket(context, ZMQ_REQ);

        socket.connect("tcp://localhost:5555");

        capnp::MallocMessageBuilder message;

        auto sessionEvent = message.initRoot<neon::session::SessionEvent>();

        std::string handle = "Test-Client-" + std::to_string(tID);
        sessionEvent.setCommand(
            neon::session::SessionEvent::Command::CREATE_SESSION);
        sessionEvent.setName(handle);

        ZMQOutputStream outStream;
        capnp::writeMessage(outStream, message);
        zmq::message_t request(outStream.data(), outStream.size(),
                               &ZMQOutputStream::release,
                               static_cast<void *>(&outStream));
        socket.send(request);

        //  Get the reply.
        zmq::message_t reply;
        socket.recv(&reply);
        char *buffer = static_cast<char *>(malloc(reply.size() + 1));
        memcpy(buffer, reply.data(), reply.size());
        buffer[reply.size()] = 0x00;
        std::string replyData(buffer);
        {
          std::scoped_lock<std::mutex> lock(countMutex);
          std::cout << ++count << " : ";
          if (replyData == handle) {
            std::cout << "Passed" << std::endl;
          } else {
            std::cout << "Failed! " << replyData << "!=" << handle
                      << std::endl;
            MismatchedReply e;
            throw e;
          }
        }
        free(buffer);
      });
    }

    for (int tID = 0; tID < THREAD_COUNT; tID++) {
      threadPool[tID].join();
    }
  }
  return 0;
}
