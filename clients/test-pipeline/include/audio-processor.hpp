#include "capnp/ez-rpc.h"
#include "capnp/serialize.h"
#include "debuglogger.hpp"
#include "neon.session.capnp.h"
#include <ZMQOutputStream.hpp>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <kj/array.h>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <zmq.hpp>

//////////////////////////////////////////////////////////////////////
//
// Audio processor definition
//

class AudioProcessor {
public:
  AudioProcessor();
  ~AudioProcessor();

  void processAudio(const capnp::byte *musicData,
                    const size_t musicDataLength) const noexcept;

  [[nodiscard]] const uint32_t getSampleRate() const noexcept;
  [[nodiscard]] const uint8_t getChannels() const noexcept;
  [[nodiscard]] const uint8_t getWidth() const noexcept;
  [[nodiscard]] const uint32_t getDurationMs() const noexcept;

private:
  const std::string handle;
  std::string sessionUUID;

  const uint32_t defaultSampleRate;
  const uint8_t defaultChannels;
  const uint8_t defaultWidth;
  const uint32_t defaultDurationMs;

  mutable std::mutex audioQueueMutex;
  mutable std::vector<std::vector<capnp::byte>> audioQueue;

  bool audioProcessorThreadRunning;
  std::thread audioProcessorThread;
  std::mutex audioProcessorWakeupMutex;
  mutable std::condition_variable audioProcessorWakeup;

  DebugLogger logger;

  void audioProcessorLoop();
};