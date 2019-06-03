#include "pulse_audio_stream.h"
#include <pthread.h>
#include <unistd.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  NeonPulseInput Static items...
//

pa_threaded_mainloop * NeonPulseInput::pulseAudioThread = nullptr;
pa_mainloop_api * NeonPulseInput::pulseAudioApi = nullptr;
pa_context * NeonPulseInput::pulseAudioContext = nullptr;
pa_context_state_t NeonPulseInput::currentState = {};
uint32_t NeonPulseInput::lockCounter = 0;
pthread_mutex_t NeonPulseInput::pulseAudioContextMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
QList<uint32_t> NeonPulseInput::moduleTrashList;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NeonPulseInput Constructor/Destructor
//

NeonPulseInput::NeonPulseInput() {}

NeonPulseInput::~NeonPulseInput() {}

void NeonPulseInput::ModuleLoadCallback(pa_context *c, uint32_t idx,
                                        void *userdata) {
  (void)c;

  NeonPulseInput *context = (NeonPulseInput *)userdata;
  context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                           "Loaded : module-pipe-source [%08x]", idx);
  if (idx != 0xFFFFFFFF) {
    context->fifoModule = idx;
  }
}

void NeonPulseInput::ModuleUnloadCallback(pa_context *c, int success,
                                          void *userdata) {
  (void)c;

  NeonPulseInput *context = (NeonPulseInput *)userdata;
  if (success) {
    NeonPulseInput *context = (NeonPulseInput *)userdata;
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Unloaded : module-pipe-source [%08x]",
                             context->fifoModule);
  } else {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Error unloading : module-pipe-source [%08x]",
                             context->fifoModule);
  }
  context->fifoModule = 0;

  // TODO : Nofity client that remote end died
  // context->DestroyStream();
}

void NeonPulseInput::ModuleLoopbackLoadCallback(pa_context *c, uint32_t idx,
                                                void *userdata) {
  (void)c;
  NeonPulseInput *context = (NeonPulseInput *)userdata;
  if (idx != 0xFFFFFFFF) {
    context->loopbackModule = idx;
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Loaded : module-loopback [%08x]",
                             context->loopbackModule);
  } else {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                             "Failed to load : module-loopback [%d]",
                             context->loopbackModule);
    context->loopbackModule = 0;
  }
}

void NeonPulseInput::ModuleLoopbackUnloadCallback(pa_context *c, int success,
                                                  void *userdata) {
  (void)c;

  NeonPulseInput *context = (NeonPulseInput *)userdata;
  if (success) {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Unloaded : module-loopback [%08x]",
                             context->loopbackModule);
  } else {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                             "Failed to unload : module-loopback [%d]",
                             context->loopbackModule);
  }
  context->loopbackModule = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NeonPulseInput Connect/Disconnect
//

bool NeonPulseInput::PAConnect() {
  if (pulseAudioThread != nullptr) {
    return true;
  }

  LockPAAPI(__PRETTY_FUNCTION__);

  pulseAudioThread = pa_threaded_mainloop_new();

  if (pulseAudioThread == NULL) {
    qDebug() << "Error connecting to Pulse Audio";
    return false;
  }

  pa_threaded_mainloop_start(pulseAudioThread);

  UnlockPAAPI(__PRETTY_FUNCTION__);

  Lock();
  pulseAudioApi = pa_threaded_mainloop_get_api(pulseAudioThread);

  pulseAudioContext = pa_context_new(pulseAudioApi, "car-play-pulse-audio");
  pa_context_set_state_callback(pulseAudioContext,
                                &NeonPulseInput::stateChangeCallback, NULL);
  pa_context_connect(pulseAudioContext, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
  Unlock();

  // Wait for ready state
  uint8_t waitCounter = 0;
  while (currentState != PA_CONTEXT_READY) {
    // qDebug() << QString("Waiting for PulseAudio context connection... (%1)")
    //                 .arg(waitCounter);
    usleep(100 * 1000);
    waitCounter++;
    if (waitCounter == 600) // One minute...?!
    {
      qDebug() << "PulseAudio context connection timeout.";
      return false;
    }
    if (currentState == PA_CONTEXT_FAILED) {
      qDebug() << "PulseAudio context connection failed.";
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

  while (!moduleTrashList.isEmpty()) {
    uint32_t index = moduleTrashList.takeLast();
    // qDebug() << QString("Unloading module [%1]").arg(index);
    Lock();
    operation = pa_context_unload_module(pulseAudioContext, index, NULL, NULL);
    Unlock();
    WaitForOp(operation);
  }

  return true;
}

bool NeonPulseInput::Connect() {

  // qDebug() << QString("Connecting to Pulse Audio");

  pthread_condattr_t attr;
  pthread_condattr_init(&attr);
  pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

  pthread_cond_init(&chunkCondition, &attr);
  pthread_mutex_init(&chunkMutex, NULL);

  pthread_cond_init(&streamCondition, &attr);
  pthread_mutex_init(&streamMutex, NULL);

  return true;
}

///////////////////////////////////////////////////////////
//
// PA Context state helper
//

pa_context_state_t NeonPulseInput::UpdateCurrentState(pa_context *context) {
  LockPAAPI(__PRETTY_FUNCTION__);
  currentState = pa_context_get_state(context);
  UnlockPAAPI(__PRETTY_FUNCTION__);
  return currentState;
}

///////////////////////////////////////////////////////////
//
// Threaded PulseAudio inferface calls
//

void NeonPulseInput::Lock() {
  LockPAAPI(__PRETTY_FUNCTION__);
  pa_threaded_mainloop_lock(pulseAudioThread);
  lockCounter++;
}

void NeonPulseInput::Unlock() {
  assert(lockCounter != 0);
  pa_threaded_mainloop_unlock(pulseAudioThread);
  UnlockPAAPI(__PRETTY_FUNCTION__);
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
      qDebug() << QString(
                      "PulseAudio command response is taking too long! %1 ms")
                      .arg(waitCounter * 100);
    }
    if (waitCounter == 50) {
      qDebug() << "PulseAudio command response timed out. 5 seconds.";
    }
  }

  LockPAAPI(__PRETTY_FUNCTION__);
  pa_operation_unref(operation);
  UnlockPAAPI(__PRETTY_FUNCTION__);
}

void NeonPulseInput::CreateStream(uint32_t rate, uint8_t bits, uint8_t channels,
                                  const char *name, bool bigEndian, bool sign) {
  if (!sign) {
    qDebug() << "Unable to create unsigned stream!";
    return;
  }

  // qDebug() << QString("Creating module-pipe-source");

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
    qDebug() << QString("Unknown number of bit depth [%1]").arg(bits);
    break;
  }

  if (channels == 1) {
    channelmap = "mono";
  } else if (channels == 2) {
    channelmap = "left,right";

  } else {
    channelmap = "";
    qDebug() << QString("Unknown number of channels [%1]").arg(channels);
  }

  frameSize = (bits / 8) * channels;

  char argBuffer[1024];
  memset(argBuffer, 0, 1024);
  if (strlen(name) == 0) {
    name = "alt";
  }

  memset(nameBuffer, 0, 256);
  snprintf(nameBuffer, 256, "%s", name);

  snprintf(argBuffer, 1024,
           "source_name=%s format=%s rate=%d channels=%d channel_map=%s "
           "file=/tmp/%s.fifo",
           nameBuffer, format, rate, channels, channelmap, nameBuffer);

  LockPAAPI(__PRETTY_FUNCTION__);

  // Load module-pipe-source
  // qDebug() << QString("Loading module-pipe-source [%1]").arg(argBuffer);
  pa_context_load_module(pulseAudioContext, "module-pipe-source", argBuffer,
                         NeonPulseInput::ModuleLoadCallback, this);

  UnlockPAAPI(__PRETTY_FUNCTION__);

  streamReadRunning = true;
  streamChunkOutputTime = 0;
}

void NeonPulseInput::DestroyStream() {
  if (fifoModule != 0) {
    // qDebug() << QString("Unloading module-pipe-source [%1]").arg(fifoModule);

    Lock();
    pa_operation *operation =
        pa_context_unload_module(pulseAudioContext, fifoModule,
                                 NeonPulseInput::ModuleUnloadCallback, this);
    Unlock();
    WaitForOp(operation);
  }
}

void NeonPulseInput::StartStream() {
  // qDebug() << QString("Stream : Start Stream");

  QString filepath = QString("/tmp/%1.fifo").arg(nameBuffer);

  // Wait for FIFO to exist...
  int timer = 0;
  while (!QFile::exists(filepath)) {
    usleep(20 * 1000);
    if (timer >= 50)
      qDebug() << QString("Waiting for fifo [%1] [%2 ms]")
                      .arg(filepath.split("/").takeLast())
                      .arg(++timer * 20);
  }

  // Load module-loopback
  char argBuffer[1024];
  memset(argBuffer, 0, 1024);
  snprintf(argBuffer, 1024, "source=%s sink_input_properties=\"media.name=%s\"",
           nameBuffer, nameBuffer);

  // qDebug() << QString("Loading module-loopback [%1]").arg(argBuffer);

  LockPAAPI(__PRETTY_FUNCTION__);
  pa_context_load_module(pulseAudioContext, "module-loopback", argBuffer,
                         NeonPulseInput::ModuleLoopbackLoadCallback, this);
  UnlockPAAPI(__PRETTY_FUNCTION__);

  // Open Stream FIFO
  fifoFile = new QFile(filepath);
  if (fifoFile->open(QIODevice::WriteOnly)) {
    // qDebug() << QString("Opened module-pipe-source FIFO");

    // qDebug() << QString("Starting Writer Thread");
    pthread_attr_t threadAttributes;
    sched_param threadScheduleParameters;
    pthread_attr_init(&threadAttributes);
    pthread_attr_setinheritsched(&threadAttributes, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&threadAttributes, SCHED_FIFO);
    pthread_attr_getschedparam(&threadAttributes, &threadScheduleParameters);
    threadScheduleParameters.sched_priority =
        sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&threadAttributes, &threadScheduleParameters);
    if (pthread_create(&writerThread, /*&threadAttributes*/ nullptr,
                       NeonPulseInput::DataStreamWriter, this)) {
      qDebug() << "Failed to create DataStreamWriter thread : " << errno;
    }

  } else {
    delete fifoFile;
    fifoFile = nullptr;
    qDebug() << QString("Failed to open module-pipe-source FIFO");
  }
}

void NeonPulseInput::StopStream() {
  // qDebug() << QString("Closed module-pipe-source FIFO");

  // Close Stream FIFO
  if (fifoFile != nullptr) {
    fifoFile->close();
  }

  streamReadRunning = false;

  if (loopbackModule != 0) {
    // qDebug() << QString("Unloading module-loopback
    // [%1]").arg(loopbackModule);

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
  // qDebug() << "Adding chunk : " << length;
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

void *NeonPulseInput::DataStreamWriter(void *arg) {
  // qDebug() << "DataStreamWriter Start";

  NeonPulseInput *parent = (NeonPulseInput *)arg;
  struct timespec timeDelay;

  /*if (streamChunkBufferHead == streamChunkBufferTail)
  {
      pendingStreamData = true;
      //qDebug() << QString("Stream Request Data : Queue empty!");
  }
  else
  {*/
  usleep(50 * 1000);

  QElapsedTimer eTimer;
  eTimer.start();
  clock_gettime(CLOCK_MONOTONIC, &timeDelay);
  uint32_t bytesWritten = 0;
  while (parent->streamReadRunning) {
    uint32_t bufferSize = 0;
    uint32_t writtenSize = 0;
    // qDebug() << "DataStreamWriter : loop" << bufferSize;
    if ((parent->fifoFile != nullptr) && parent->fifoFile->isOpen()) {
      parent->fifoLock.lock();
      bufferSize = parent->fifoQueue.size();
      if (bufferSize > 0) {
        // qDebug() << "Writing to queue : " << bufferSize;
        writtenSize = parent->fifoFile->write(parent->fifoQueue);
        parent->fifoQueue.clear();
      }
      parent->fifoLock.unlock();
    }

    if (writtenSize > 0) {
      // qDebug() << QString("Data Writer : Wrote [%1 bytes]").arg(writtenSize);
      parent->streamChunkOutputTime += (writtenSize / parent->frameSize);
      bytesWritten += writtenSize;
      if (writtenSize != bufferSize) {
        qDebug() << QString(
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

    // qDebug() << QString("Data Writer : Alseep");

    pthread_mutex_lock(&parent->chunkMutex);
    pthread_cond_timedwait(&parent->chunkCondition, &parent->chunkMutex,
                           &timeDelay);
    // pthread_cond_wait(&parent->chunkCondition, &parent->chunkMutex);
    pthread_mutex_unlock(&parent->chunkMutex);

    // qDebug() << QString("Data Writer : Awake");

    if (eTimer.elapsed() > 1000) {
      eTimer.restart();
      // qDebug() << QString("Data Writer : Status : [%1 bytes written]")
      //                 .arg(bytesWritten);
      bytesWritten = 0;
    }
  }

  // qDebug() << "DataStreamWriter exit";
  return nullptr;
}