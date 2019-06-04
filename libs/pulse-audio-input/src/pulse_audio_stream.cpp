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

NeonPulseInput *NeonPulseInput::instance = nullptr;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NeonPulseInput Constructor/Destructor
//

NeonPulseInput::NeonPulseInput()
    : streamName(std::string("PulseAudioInput-") + std::to_string(getpid())),
      logger(streamName, DebugLogger::DebugColor::COLOR_CYAN, false),
      frameSize(0), pulseAudioThread(nullptr), pulseAudioApi(nullptr),
      pulseAudioContext(nullptr), currentState(PA_CONTEXT_UNCONNECTED),
      pcmStream(nullptr), lockCounter(0) {
  instance = this;
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Created Pulse Audio Input");
}

NeonPulseInput::~NeonPulseInput() { instance = nullptr; }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NeonPulseInput Connect/Disconnect
//

bool NeonPulseInput::Connect() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Connecting to Pulse Audio Source");

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
  uint32_t waitCounter = 0;
  while (currentState != PA_CONTEXT_READY) {
    if (waitCounter > 5) {
      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                      "Waiting for PulseAudio context connection... [%d]",
                      waitCounter);
    }
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

void NeonPulseInput::Disconnect() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Disconnecting from Pulse Audio Source");
}

void NeonPulseInput::CreateStream(const uint8_t channels, const uint32_t rate,
                                  const pa_sample_format_t format) {
  if (pcmStream != nullptr) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_ERROR,
                    "PCM stream already exists...");
    return;
  }

  pa_sample_spec sampleSpec;
  sampleSpec.channels = channels;
  sampleSpec.rate = rate;
  sampleSpec.format = format;

  pcmStream = pa_stream_new(pulseAudioContext, streamName.c_str(), &sampleSpec,
                            nullptr);
  if (!pcmStream) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_ERROR,
                    "Failed to create PCM recording stream with the following "
                    "settings [%d channels] [%d sample rate] [%s]",
                    channels, rate, audioFormatStrings.at(format).c_str());
    return;
  }

  pa_stream_set_read_callback(pcmStream, &NeonPulseInput::audioDataCallback,
                              nullptr);

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Created PCM recording stream with the following "
                  "settings [%d channels] [%d sample rate] [%s]",
                  channels, rate, audioFormatStrings.at(format).c_str());

  if (pa_stream_connect_record(pcmStream, nullptr, nullptr,
                               PA_STREAM_NOFLAGS)) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Failed to record from PCM stream");
  } else {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                    "Started Recording from PCM stream");
  }
}

void NeonPulseInput::DestroyStream() {
  if (!pcmStream) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Can not destroy uninitialized PCM stream");
  }

  if (pa_stream_disconnect(pcmStream)) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Failed to disconnect PCM stream");

  } else {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                    "Disconnected PCM stream");
  }

  pa_stream_unref(pcmStream);
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Destroyed PCM stream");
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