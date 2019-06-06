//
//  Hello World client in C++
//  Connects REQ socket to tcp://localhost:5555
//  Sends "Hello" to server, expects "World" back
//
#include "capnp/ez-rpc.h"
#include "capnp/serialize.h"
#include "neon.session.capnp.h"
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
        {
          capnp::EzRpcClient client("localhost:5554");
          neon::session::Controller::Client controllerServer =
              client.getMain<neon::session::Controller>();
          auto &waitScope = client.getWaitScope();
          std::string handle = "Test-Client-" + std::to_string(tID);
          auto request = controllerServer.createSessionRequest();
          request.setName(handle);
          auto promise = request.send();
          auto response = promise.wait(waitScope);

          std::string replyData = response.getUuid();

          std::scoped_lock<std::mutex> lock(countMutex);
          std::cout << ++count << " : ";
          if (replyData == handle) {
            std::cout << "Passed" << std::endl;
          } else {
            std::cout << "Failed! " << replyData << "!=" << handle << std::endl;
            MismatchedReply e;
            throw e;
          }
        }
      });
    }

    for (int tID = 0; tID < THREAD_COUNT; tID++) {
      threadPool[tID].join();
    }
  }
  return 0;
}
