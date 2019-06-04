#include "pulse_audio_stream.hpp"
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  NeonPulseInput Static items...
//

pa_threaded_mainloop *NeonPulseInput::pulseAudioThread = nullptr;
pa_mainloop_api *NeonPulseInput::pulseAudioApi = nullptr;
pa_context *NeonPulseInput::pulseAudioContext = nullptr;
pa_context_state_t NeonPulseInput::currentState = {};
uint32_t NeonPulseInput::lockCounter = 0;
pthread_mutex_t NeonPulseInput::pulseAudioContextMutex =
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NeonPulseInput Constructor/Destructor
//

NeonPulseInput::NeonPulseInput()
    : logger("PulseAudioInput-" + std::to_string(getpid()),
             DebugLogger::DebugColor::COLOR_CYAN, false),
      streamReadRunning(false), frameSize(0), fifoFile(0) {}

NeonPulseInput::~NeonPulseInput() { instance = nullptr; }

void NeonPulseInput::ModuleLoadCallback(pa_context *c, uint32_t idx,
                                        void *userdata) {
  (void)c;
  (void)userdata;

  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                            "Loaded : module-pipe-source [%08x]", idx);
  if (idx != 0xFFFFFFFF) {
    instance->fifoModule = idx;
  }
}

void NeonPulseInput::ModuleUnloadCallback(pa_context *c, int success,
                                          void *userdata) {
  (void)c;
  (void)userdata;

  if (success) {
    NeonPulseInput *context = (NeonPulseInput *)userdata;
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Unloaded : module-pipe-source [%08x]",
                              instance->fifoModule);
  } else {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Error unloading : module-pipe-source [%08x]",
                              instance->fifoModule);
  }
  instance->fifoModule = 0;

  // TODO : Nofity client that remote end died
  // TODO: Is this needed?
  // instance->DestroyStream();
}

void NeonPulseInput::ModuleLoopbackLoadCallback(pa_context *c, uint32_t idx,
                                                void *userdata) {
  (void)c;
  (void)userdata;

  if (idx != 0xFFFFFFFF) {
    instance->loopbackModule = idx;
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Loaded : module-loopback [%08x]",
                              instance->loopbackModule);
  } else {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                              "Failed to load : module-loopback [%d]",
                              instance->loopbackModule);
    instance->loopbackModule = 0;
  }
}

void NeonPulseInput::ModuleLoopbackUnloadCallback(pa_context *c, int success,
                                                  void *userdata) {
  (void)c;
  (void)userdata;

  if (success) {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Unloaded : module-loopback [%08x]",
                              instance->loopbackModule);
  } else {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                              "Failed to unload : module-loopback [%d]",
                              instance->loopbackModule);
  }
  instance->loopbackModule = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NeonPulseInput Connect/Disconnect
//

bool NeonPulseInput::PAConnect() {
  if (pulseAudioThread != nullptr) {
    return true;
  }

  pulseAudioThread = pa_threaded_mainloop_new();

  if (pulseAudioThread == NULL) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Error connecting to Pulse Audio");
    return false;
  }

  pa_threaded_mainloop_start(pulseAudioThread);

  Lock();
  pulseAudioApi = pa_threaded_mainloop_get_api(pulseAudioThread);

  pulseAudioContext = pa_context_new(pulseAudioApi, "pulse-audio-input");
  pa_context_set_state_callback(pulseAudioContext,
                                &NeonPulseInput::stateChangeCallback, NULL);
  pa_context_connect(pulseAudioContext, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
  Unlock();

  // Wait for ready state
  uint8_t waitCounter = 0;
  while (currentState != PA_CONTEXT_READY) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Waiting for PulseAudio context connection... [%d]",
                    waitCounter);
    usleep(100 * 1000);
    waitCounter++;
    if (waitCounter == 600) // One minute...?!
    {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_ERROR,
                      "PulseAudio context connection timeout.");
      return false;
    }
    if (currentState == PA_CONTEXT_FAILED) {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_ERROR,
                      "PulseAudio context connection failed.");
      return false;
    }
  }

  // Get server info
  pa_operation *operation;

  Lock();
  operation = pa_context_get_server_info(
      pulseAudioContext, &NeonPulseInput::serverInfoCallback, NULL);
  Unlock();
  WaitForOp(operation);

  Lock();
  operation = pa_context_get_module_info_list(
      pulseAudioContext, &NeonPulseInput::moduleInfoCallback, NULL);
  Unlock();
  WaitForOp(operation);

  return true;
}

bool NeonPulseInput::Connect() {

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Connecting to Pulse Audio Source");

  pthread_condattr_t attr;
  pthread_condattr_init(&attr);
  pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

  // pthread_cond_init(&chunkCondition, &attr);
  // pthread_mutex_init(&chunkMutex, NULL);

  // pthread_cond_init(&streamCondition, &attr);
  // pthread_mutex_init(&streamMutex, NULL);

  return true;
}

///////////////////////////////////////////////////////////
//
// PA Context state helper
//

pa_context_state_t NeonPulseInput::UpdateCurrentState(pa_context *context) {

  currentState = pa_context_get_state(context);

  return currentState;
}

///////////////////////////////////////////////////////////
//
// Threaded PulseAudio inferface calls
//

void NeonPulseInput::Lock() {
  assert(lockCounter == 0);
  pa_threaded_mainloop_lock(pulseAudioThread);
  lockCounter++;
}

void NeonPulseInput::Unlock() {
  assert(lockCounter != 0);
  pa_threaded_mainloop_unlock(pulseAudioThread);

  lockCounter--;
}

int NeonPulseInput::CheckLock() { return lockCounter; }

void NeonPulseInput::WaitForOp(pa_operation *operation) {
  if (operation == nullptr) {
    return;
  }

  pa_operation_state_t opState = PA_OPERATION_RUNNING;
  uint32_t waitCounter = 0;
  while (opState == PA_OPERATION_RUNNING) {
    Lock();
    opState = pa_operation_get_state(operation);
    Unlock();

    usleep(100 * 1000);

    waitCounter++;
    if (waitCounter > 5) {
      instance->logger.WriteLog(
          DebugLogger::DebugLevel::DEBUG_WARNING,
          "PulseAudio command response is taking too long! %d ms",
          waitCounter * 100);
    }
    if (waitCounter == 50) {
      instance->logger.WriteLog(
          DebugLogger::DebugLevel::DEBUG_WARNING,
          "PulseAudio command response timed out. 5 seconds.");
    }
  }

  pa_operation_unref(operation);
}

void NeonPulseInput::CreateStream(const std::string name, uint32_t rate,
                                  uint8_t bits, uint8_t channels,
                                  bool bigEndian, bool sign) {
  sourceName = name;
  if (!sign) {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                              "Unable to create unsigned stream!");
    return;
  }

  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                            "Creating module-pipe-source");

  const char *format = nullptr;
  const char *channelmap = nullptr;

  switch (bits) {
  case 16:
    format = bigEndian ? "s16be" : "s16le";
    break;
  case 24:
    format = bigEndian ? "s24be" : "s24le";
    break;
  case 32:
    format = bigEndian ? "s32be" : "s32le";
    break;
  default:
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Unknown bit depth [%d]", bits);
    break;
  }

  if (channels == 1) {
    channelmap = "mono";
  } else if (channels == 2) {
    channelmap = "left,right";

  } else {
    channelmap = "";
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Unknown number of channels [%d]", channels);
  }

  frameSize = (bits / 8) * channels;

  char argBuffer[1024];
  memset(argBuffer, 0, 1024);

  auto nameBuffer = sourceName.c_str();

  snprintf(argBuffer, 1024,
           "source_name=%s format=%s rate=%d channels=%d channel_map=%s "
           "file=/tmp/%s.fifo",
           nameBuffer, format, rate, channels, channelmap, nameBuffer);

  // Load module-pipe-source
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Loading module-pipe-source [%s]", argBuffer);
  pa_context_load_module(pulseAudioContext, "module-pipe-source", argBuffer,
                         NeonPulseInput::ModuleLoadCallback, this);
}

void NeonPulseInput::DestroyStream() {
  if (fifoModule != 0) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Unloading module-pipe-source [%d]", fifoModule);

    Lock();
    pa_operation *operation =
        pa_context_unload_module(pulseAudioContext, fifoModule,
                                 NeonPulseInput::ModuleUnloadCallback, this);
    Unlock();
    WaitForOp(operation);
  }
}

void NeonPulseInput::StartStream() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Start Stream");

  std::string filepath = std::string("/tmp/") + sourceName + (".fifo");

  // Wait for FIFO to exist...
  int timer = 0;
  struct stat fifoStat;
  while (stat(filepath.c_str(), &fifoStat) == -1) {
    usleep(20 * 1000);
    if (timer >= 50)
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                      "Waiting for fifo [%s] [%d ms]", filepath.c_str(),
                      timer * 20);
    timer++;
  }

  // Load module-loopback
  const char *nameBuffer = sourceName.c_str();
  char argBuffer[1024];
  memset(argBuffer, 0, 1024);
  snprintf(argBuffer, 1024, "source=%s sink_input_properties=\"media.name=%s\"",
           nameBuffer, nameBuffer);

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                  "Loading module-loopback [%s]", argBuffer);

  pa_context_load_module(pulseAudioContext, "module-loopback", argBuffer,
                         NeonPulseInput::ModuleLoopbackLoadCallback, this);

  // Open Stream FIFO
  fifoFile = open(filepath.c_str(), O_RDONLY);
  if (fifoFile != -1) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Opened module-pipe-source FIFO [%d]", fifoFile);

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Starting Reader Thread");

    pthread_attr_t threadAttributes;
    sched_param threadScheduleParameters;
    pthread_attr_init(&threadAttributes);
    pthread_attr_setinheritsched(&threadAttributes, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&threadAttributes, SCHED_FIFO);
    pthread_attr_getschedparam(&threadAttributes, &threadScheduleParameters);
    threadScheduleParameters.sched_priority =
        sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&threadAttributes, &threadScheduleParameters);
    if (pthread_create(&readerThread, /*&threadAttributes*/ nullptr,
                       NeonPulseInput::DataStreamReader, this)) {
      instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                "Failed to create DataStreamReader thread [%s]",
                                errno);
    }

  } else {
    fifoFile = 0;
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_ERROR,
                    "Failed to open module-pipe-source FIFO [%s]",
                    strerror(errno));
  }
}

void NeonPulseInput::StopStream() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Closing module-pipe-source FIFO [%d]", fifoFile);

  // Close Stream FIFO
  close(fifoFile);
  fifoFile = -1;

  streamReadRunning = false;

  if (loopbackModule != 0) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Unloading module-loopback [%d]", loopbackModule);

    Lock();
    pa_operation *operation = pa_context_unload_module(
        pulseAudioContext, loopbackModule,
        NeonPulseInput::ModuleLoopbackUnloadCallback, this);
    Unlock();
    WaitForOp(operation);
  }
}

void NeonPulseInput::AddStreamChunk(uint8_t *data, uint32_t length) {
  // Write data to FIFO
  fifoLock.lock();
  // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,"Adding
  // chunk : " << length;
  fifoQueue.append((const char *)data, (int)length);
  totalBytes += length;
  fifoLock.unlock();
  // streamChunkOutputTime += (length / frameSize);
  // free(data);
}

uint32_t NeonPulseInput::GetSampleTime() { return streamChunkOutputTime; }

uint32_t NeonPulseInput::GetTotalBytes() { return totalBytes; }

void NeonPulseInput::ProcessStreamData() {
  pthread_cond_signal(&chunkCondition);
}

void *NeonPulseInput::DataStreamReader(void *arg) {
  // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,"DataStreamReader
  // Start";

  NeonPulseInput *parent = (NeonPulseInput *)arg;
  struct timespec timeDelay;

  /*if (streamChunkBufferHead == streamChunkBufferTail)
  {
      pendingStreamData = true;
      //instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,QString("Stream
  Request Data : Queue empty!");
  }
  else
  {*/
  usleep(50 * 1000);

  clock_gettime(CLOCK_MONOTONIC, &timeDelay);
  parent->streamReadRunning = true;
  uint32_t bytesWritten = 0;
  while (parent->streamReadRunning) {
    uint32_t bufferSize = 0;
    uint32_t writtenSize = 0;
    // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,"DataStreamReader
    // : loop" << bufferSize;
    if ((parent->fifoFile != nullptr) && parent->fifoFile->isOpen()) {
      parent->fifoLock.lock();
      bufferSize = parent->fifoQueue.size();
      if (bufferSize > 0) {
        // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,"Writing
        // to queue : " << bufferSize;
        writtenSize = parent->fifoFile->write(parent->fifoQueue);
        parent->fifoQueue.clear();
      }
      parent->fifoLock.unlock();
    }

    if (writtenSize > 0) {
      // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,QString("Data
      // Reader : Wrote [%1 bytes]").arg(writtenSize);
      parent->streamChunkOutputTime += (writtenSize / parent->frameSize);
      bytesWritten += writtenSize;
      if (writtenSize != bufferSize) {
        instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,QString(
                        "Failed to write to module-pipe-source FIFO [%1]!=[%2]")
                        .arg(writtenSize)
                        .arg(bufferSize);
      }
    }

    timeDelay.tv_nsec += 60 * 1E6; // 50 ms
    if (timeDelay.tv_nsec >= 1E9) {
      timeDelay.tv_nsec -= 1E9;
      timeDelay.tv_sec++;
    }

    // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,QString("Data
    // Reader : Alseep");

    pthread_mutex_lock(&parent->chunkMutex);
    pthread_cond_timedwait(&parent->chunkCondition, &parent->chunkMutex,
                           &timeDelay);
    // pthread_cond_wait(&parent->chunkCondition, &parent->chunkMutex);
    pthread_mutex_unlock(&parent->chunkMutex);

    // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,QString("Data
    // Reader : Awake");

    if (eTimer.elapsed() > 1000) {
      eTimer.restart();
      // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,QString("Data
      // Reader : Status : [%1 bytes written]")
      //                 .arg(bytesWritten);
      bytesWritten = 0;
    }
  }

  // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,"DataStreamReader
  // exit";
  return nullptr;
}