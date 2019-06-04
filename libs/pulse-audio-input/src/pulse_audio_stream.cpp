#include "pulse_audio_stream.hpp"
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
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
NeonPulseInput *NeonPulseInput::instance = nullptr;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NeonPulseInput Constructor/Destructor
//

NeonPulseInput::NeonPulseInput()
    : logger("PulseAudioInput-" + std::to_string(getpid()),
             DebugLogger::DebugColor::COLOR_CYAN, false),
      streamReadRunning(false), frameSize(0), fifoFile(0) {
  instance = this;
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Created Pulse Audio Input");
}

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
                              "Error unloading : module-loopback [%d]",
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

  Lock();
  operation = pa_context_get_source_info_list(
      pulseAudioContext, &NeonPulseInput::sourceInfoCallback, NULL);
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

void NeonPulseInput::Disconnect() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Disconnecting from Pulse Audio Source");

  // Last chance for clean up of modules...

  if (loopbackModule != 0) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Unloading module-loopback [%08x]", loopbackModule);

    Lock();
    pa_operation *operation = pa_context_unload_module(
        pulseAudioContext, fifoModule,
        NeonPulseInput::ModuleLoopbackUnloadCallback, nullptr);
    Unlock();
    WaitForOp(operation);
  }

  if (fifoModule != 0) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Unloading module-pipe-source [%08x]", fifoModule);

    Lock();
    pa_operation *operation =
        pa_context_unload_module(pulseAudioContext, fifoModule,
                                 NeonPulseInput::ModuleUnloadCallback, nullptr);
    Unlock();
    WaitForOp(operation);
  }
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

void NeonPulseInput::addSource(const pa_source_info *info) {
  instance->logger.WriteLog(
      DebugLogger::DebugLevel::DEBUG_INFO,
      "Source [%d %s: %s] [%d channels] [%d Hz] [%s]", info->index, info->name,
      info->description, info->sample_spec.channels, info->sample_spec.rate,
      audioFormatStrings.at(info->sample_spec.format).c_str());

  NeonAudioFormat format;
  format.name = std::string(info->name);
  format.rate = info->sample_spec.rate;
  format.channels = info->sample_spec.channels;
  switch (info->sample_spec.format) {
  case PA_SAMPLE_U8:
    format.bits = 8;
    format.bigEndian = false;
    format.sign = false;
    break;
  case PA_SAMPLE_ALAW:
    format.bits = 8;
    format.bigEndian = false;
    format.sign = false;
    break;
  case PA_SAMPLE_ULAW:
    format.bits = 8;
    format.bigEndian = false;
    format.sign = false;
    break;
  case PA_SAMPLE_S16LE:
    format.bits = 16;
    format.bigEndian = false;
    format.sign = true;
    break;
  case PA_SAMPLE_S16BE:
    format.bits = 16;
    format.bigEndian = true;
    format.sign = true;
    break;
  case PA_SAMPLE_FLOAT32LE:
    format.bits = 32;
    format.bigEndian = false;
    format.sign = true;
    break;
  case PA_SAMPLE_FLOAT32BE:
    format.bits = 32;
    format.bigEndian = true;
    format.sign = true;
    break;
  case PA_SAMPLE_S32LE:
    format.bits = 32;
    format.bigEndian = false;
    format.sign = true;
    break;
  case PA_SAMPLE_S32BE:
    format.bits = 32;
    format.bigEndian = true;
    format.sign = true;
    break;
  case PA_SAMPLE_S24LE:
    format.bits = 24;
    format.bigEndian = false;
    format.sign = true;
    break;
  case PA_SAMPLE_S24BE:
    format.bits = 24;
    format.bigEndian = true;
    format.sign = true;
    break;
  case PA_SAMPLE_S24_32LE:
    format.bits = 32;
    format.bigEndian = false;
    format.sign = true;
    break;
  case PA_SAMPLE_S24_32BE:
    format.bits = 32;
    format.bigEndian = true;
    format.sign = true;
    break;
  }
  neonAudioSources[info->index] = format;
}

void NeonPulseInput::CreateStream(const int sourceIndex) {
  CreateStream(neonAudioSources.at(sourceIndex));
}

void NeonPulseInput::CreateStream(const NeonAudioFormat audioFormat) {
  sourceName = audioFormat.name;
  if (!audioFormat.sign) {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                              "Unable to create unsigned stream!");
    return;
  }

  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                            "Creating module-pipe-source");

  const char *format = nullptr;
  const char *channelmap = nullptr;

  switch (audioFormat.bits) {
  case 16:
    format = audioFormat.bigEndian ? "s16be" : "s16le";
    break;
  case 24:
    format = audioFormat.bigEndian ? "s24be" : "s24le";
    break;
  case 32:
    format = audioFormat.bigEndian ? "s32be" : "s32le";
    break;
  default:
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Unknown bit depth [%d]", audioFormat.bits);
    break;
  }

  if (audioFormat.channels == 1) {
    channelmap = "mono";
  } else if (audioFormat.channels == 2) {
    channelmap = "left,right";

  } else {
    channelmap = "";
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Unknown number of channels [%d]", audioFormat.channels);
  }

  frameSize = (audioFormat.bits / 8) * audioFormat.channels;

  char argBuffer[1024];
  memset(argBuffer, 0, 1024);

  auto nameBuffer = sourceName.c_str();

  snprintf(argBuffer, 1024,
           "source_name=%s format=%s rate=%d channels=%d channel_map=%s "
           "file=/tmp/%s.fifo",
           nameBuffer, format, audioFormat.rate, audioFormat.channels,
           channelmap, nameBuffer);

  // Load module-pipe-source
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Loading module-pipe-source [%s]", argBuffer);
  pa_context_load_module(pulseAudioContext, "module-pipe-source", argBuffer,
                         NeonPulseInput::ModuleLoadCallback, this);
}

void NeonPulseInput::DestroyStream() {
  if (fifoModule != 0) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Unloading module-pipe-source [%08x]", fifoModule);

    Lock();
    pa_operation *operation =
        pa_context_unload_module(pulseAudioContext, fifoModule,
                                 NeonPulseInput::ModuleUnloadCallback, nullptr);
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

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
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
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Unloading module-loopback [%08x]", loopbackModule);

    Lock();
    pa_operation *operation = pa_context_unload_module(
        pulseAudioContext, loopbackModule,
        NeonPulseInput::ModuleLoopbackUnloadCallback, nullptr);
    Unlock();
    WaitForOp(operation);
  }
}

void *NeonPulseInput::DataStreamReader(void *arg) {
  NeonPulseInput *parent = (NeonPulseInput *)arg;
  parent->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                          "DataStreamReader Start");

  parent->streamReadRunning = true;
  uint32_t bytesWritten = 0;
  bool eof = false;
  struct timeval timeout;
  timeout.tv_usec = 50 * 1E3; // 50 ms
  timeout.tv_sec = 0;

  fd_set fdSet;
  FD_ZERO(&fdSet);
  FD_SET(parent->fifoFile, &fdSet);

  auto start = std::chrono::steady_clock::now();

  while (parent->streamReadRunning && !eof) {
    uint32_t readSize = 0;

    // parent->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
    //                         "DataStreamReader : loop");
    /* select returns 0 if timeout, 1 if input available, -1 if error. */
    int result = select(FD_SETSIZE, &fdSet, NULL, NULL, &timeout);
    if (result == 0) {
      // Timeout occured...
      continue;
    } else if (result == 1) {
      // Data is ready!
      const int bufferSize = 16;
      uint8_t buffer[bufferSize];
      ssize_t readResult = read(parent->fifoFile, &buffer, bufferSize);
      if (readResult > 0) {
        readSize += readResult;
      } else if (readResult == -1) {
        parent->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                "DataStreamReader : read failed [%s]",
                                strerror(errno));
        eof = true;
        parent->streamReadRunning = false;
        break;
      } else {
        parent->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                "DataStreamReader : read 0...?");
      }
    } else {
      // Error occured :(
      parent->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                              "DataStreamReader : select failed [%s]",
                              strerror(errno));
      eof = true;
      parent->streamReadRunning = false;
      break;
    }

    auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count() > 1000) {
      start = now;
      parent->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                              "Data Reader : Status : [%d bytes read/sec]",
                              readSize);
      readSize = 0;
    }
  }

  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                            "DataStreamReader exit");
  return nullptr;
}