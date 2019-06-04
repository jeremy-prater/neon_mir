#pragma once

#include "debuglogger.hpp"
#include <pulse/pulseaudio.h>
#include <stdint.h>
#include <unordered_map>

class NeonPulseInput {
public:
  NeonPulseInput();
  ~NeonPulseInput();

  bool Connect();
  void Disconnect();

  void CreateStream(const uint8_t channels, const uint32_t rate,
                    const pa_sample_format_t format);
  void DestroyStream();

private:
  void Lock();
  void Unlock();
  int CheckLock();
  void WaitForOp(pa_operation *operation);
  pa_context_state_t UpdateCurrentState(pa_context *context);

  const std::string streamName;
  DebugLogger logger;

  int frameSize;

  pa_threaded_mainloop *pulseAudioThread;
  pa_mainloop_api *pulseAudioApi;
  pa_context *pulseAudioContext;
  pa_context_state_t currentState;
  pa_stream *pcmStream;
  uint32_t lockCounter;

private: // Pulse Audio Callbacks and singleton
  static NeonPulseInput *instance;
  static void serverInfoCallback(pa_context *c, const pa_server_info *i,
                                 void *userdata);
  static void stateChangeCallback(pa_context *c, void *userdata);
  static void moduleInfoCallback(pa_context *c, const pa_module_info *i,
                                 int eol, void *userdata);
  static void successCallback(pa_context *context, int success, void *raw);
  static void eventCallback(pa_context *c, pa_subscription_event_type_t t,
                            uint32_t index, void *userdata);
  static void audioDataCallback(pa_stream *p, size_t nbytes, void *userdata);

  static const std::unordered_map<const pa_sample_format_t, const std::string>
      audioFormatStrings;
};
