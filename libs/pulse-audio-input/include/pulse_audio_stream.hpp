#pragma once

#include "debuglogger.hpp"
#include <pulse/pulseaudio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unordered_map>

typedef struct {
  std::string name;
  uint32_t rate;
  uint8_t bits;
  uint8_t channels;
  bool bigEndian;
  bool sign;
} NeonAudioFormat;

class NeonPulseInput {
public:
  NeonPulseInput();
  ~NeonPulseInput();

  bool PAConnect();

  bool Connect();
  void Disconnect();

  void CreateStream(const int sourceID);
  void DestroyStream();

  void StartStream();
  void StopStream();

private:
  static void Lock();
  static void Unlock();
  static int CheckLock();
  static void WaitForOp(pa_operation *operation);
  static pa_context_state_t UpdateCurrentState(pa_context *context);

  static void ModuleLoadCallback(pa_context *c, uint32_t idx, void *userdata);
  static void ModuleUnloadCallback(pa_context *c, int success, void *userdata);

  static void ModuleLoopbackLoadCallback(pa_context *c, uint32_t idx,
                                         void *userdata);
  static void ModuleLoopbackUnloadCallback(pa_context *c, int success,
                                           void *userdata);

  static void serverInfoCallback(pa_context *c, const pa_server_info *i,
                                 void *userdata);
  static void stateChangeCallback(pa_context *c, void *userdata);
  static void moduleInfoCallback(pa_context *c, const pa_module_info *i,
                                 int eol, void *userdata);
  static void sourceInfoCallback(pa_context *c, const pa_source_info *i,
                                 int eol, void *userdata);
  static void successCallback(pa_context *context, int success, void *raw);
  static void eventCallback(pa_context *c, pa_subscription_event_type_t t,
                            uint32_t index, void *userdata);

  static void *DataStreamReader(void *arg);

  DebugLogger logger;

  bool streamReadRunning;
  int frameSize;
  int fifoFile;
  std::string sourceName;
  pthread_t readerThread;

  void addSource(const pa_source_info *info);
  void CreateStream(const NeonAudioFormat format);

  std::unordered_map<uint32_t, NeonAudioFormat> neonAudioSources;

  static NeonPulseInput *instance;

  static pa_threaded_mainloop *pulseAudioThread;
  static pa_mainloop_api *pulseAudioApi;
  static pa_context *pulseAudioContext;
  static pa_context_state_t currentState;
  static uint32_t lockCounter;
  static pthread_mutex_t pulseAudioContextMutex;
  static const std::unordered_map<const pa_sample_format_t, const std::string>
      audioFormatStrings;

protected:
  uint32_t loopbackModule;
  uint32_t fifoModule;
};
