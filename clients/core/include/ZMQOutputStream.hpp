#pragma once

#include "kj/io.h"
#include "zmq.hpp"
#include <exception>
#include <iostream>

class ZMQOutputStreamException : public std::exception {
  virtual const char *what() const throw() {
    return "ZMQOutputStreamException : write : size mismatch!";
  }
};

class ZMQOutputStream : public kj::OutputStream {
public:
  ZMQOutputStream()
      : dataBlock(nullptr), dataSize(0) {}
  virtual ~ZMQOutputStream() noexcept(false) {}

  static void release(void *data_, void *hint_) {
    static_cast<ZMQOutputStream *>(hint_)->release();
  }

  virtual void write(const void *buffer, size_t size) {
    dataSize += size;
    std::cout << "Created ZMQ message : " << dataSize << "bytes" << std::endl;
    dataBlock = realloc(dataBlock, dataSize);
    void *start = static_cast<uint8_t *>(dataBlock) + dataSize - size;
    memcpy(start, buffer, size);
  }

  [[nodiscard]] inline void *data() { return dataBlock; }
  [[nodiscard]] inline const uint32_t size() { return dataSize; }

  inline void release() {
    free(dataBlock);
    dataSize = 0;
  }

private:
  void *dataBlock;
  uint32_t dataSize;
};
