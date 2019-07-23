#include "pulse_audio_stream.hpp"
#include <pulse/pulseaudio.h>
#include <pulse/utf8.h>
#include <string.h>

const std::unordered_map<const pa_sample_format_t, const std::string>
    NeonPulseInput::audioFormatStrings{
        {PA_SAMPLE_U8, "Unsigned 8 Bit PCM"},
        {PA_SAMPLE_ALAW, "8 Bit a-Law"},
        {PA_SAMPLE_ULAW, "8 Bit mu-Law"},
        {PA_SAMPLE_S16LE, "Signed 16 Bit PCM, little endian (PC)"},
        {PA_SAMPLE_S16BE, "Signed 16 Bit PCM, big endian"},
        {PA_SAMPLE_FLOAT32LE,
         "32 Bit IEEE floating point, little endian (PC), range -1.0 to 1.0"},
        {PA_SAMPLE_FLOAT32BE,
         "32 Bit IEEE floating point, big endian, range -1.0 to 1.0"},
        {PA_SAMPLE_S32LE, "Signed 32 Bit PCM, little endian (PC)"},
        {PA_SAMPLE_S32BE, "Signed 32 Bit PCM, big endian"},
        {PA_SAMPLE_S24LE, "Signed 24 Bit PCM packed, little endian (PC)"},
        {PA_SAMPLE_S24BE, "Signed 24 Bit PCM packed, big endian"},
        {PA_SAMPLE_S24_32LE,
         "Signed 24 Bit PCM in LSB of 32 Bit words, little endian (PC)"},
        {PA_SAMPLE_S24_32BE,
         "Signed 24 Bit PCM in LSB of 32 Bit words, big endian"}};

void NeonPulseInput::serverInfoCallback(pa_context *c, const pa_server_info *i,
                                        void *userdata) {
  (void)c;
  (void)userdata;

  NeonPulseInput *context = (NeonPulseInput *)userdata;

  if (i != NULL) {
    // Display server info in debug log
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Pulse Audio Server Information");
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              " -- Username       : %s", i->user_name);
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              " -- Hostname       : %s", i->host_name);
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              " -- Server Version : %s", i->server_version);
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              " -- Server Name    : %s", i->server_name);
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              " -- Default Sink   : %s", i->default_sink_name);
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              " -- Default Source : %s",
                              i->default_source_name);
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              " -- Cookie         : 0x%08x", i->cookie);
  }
}

void NeonPulseInput::moduleInfoCallback(pa_context *c, const pa_module_info *i,
                                        int eol, void *userdata) {
  (void)c;

  if (eol != 0) {
    return;
  }

  NeonPulseInput *context = (NeonPulseInput *)userdata;

  // instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
  //                           "Module [%d: %s %s] is loaded and %s", i->index,
  //                           i->name, i->argument,
  //                           i->n_used == PA_INVALID_INDEX ? "un-used" :
  //                           "used");
}

void NeonPulseInput::stateChangeCallback(pa_context *c, void *userdata) {
  switch (instance->UpdateCurrentState(c)) {
  case PA_CONTEXT_READY: {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Context state: PA_CONTEXT_READY");
  } break;
  case PA_CONTEXT_FAILED: {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Context state: PA_CONTEXT_FAILED");
  } break;
  case PA_CONTEXT_UNCONNECTED: {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Context state: PA_CONTEXT_UNCONNECTED");
  } break;
  case PA_CONTEXT_AUTHORIZING: {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Context state: PA_CONTEXT_AUTHORIZING");
  } break;
  case PA_CONTEXT_SETTING_NAME: {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Context state: PA_CONTEXT_SETTING_NAME");
  } break;
  case PA_CONTEXT_CONNECTING: {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Context state: PA_CONTEXT_CONNECTING");
  } break;
  case PA_CONTEXT_TERMINATED: {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                              "Context state: PA_CONTEXT_TERMINATED");
  } break;
  }
}

void NeonPulseInput::eventCallback(pa_context *c,
                                   pa_subscription_event_type_t t,
                                   uint32_t index, void *userdata) {
  (void)c;
  instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                            "Pulse Audio Event: [0x%08x] -> [%d]",
                            t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK, index);
}

void NeonPulseInput::successCallback(pa_context *context, int success,
                                     void *userdata) {
  (void)context;

  if (!success) {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                              "Pulse Audio Command Failed");
  }
}

void NeonPulseInput::audioDataCallback(pa_stream *p, size_t nbytes,
                                       void *userdata) {
  const float *audioData;
  size_t audioDataSize;
  int peekResult = pa_stream_peek(
      p, reinterpret_cast<const void **>(&audioData), &audioDataSize);

  if (peekResult) {
    instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                              "Failed to peek into PCM stream [%d]",
                              peekResult);
    return;
  }

  if (!audioData) {
    // Could be empty or hole...
    if (audioDataSize) {
      // This is a hole.
      // We need to drop audioDataSize
      pa_stream_drop(p);
      instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                "client audio dropping [%d bytes]",
                                audioDataSize);
    } else {
      // Buffer is just empty
      instance->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                "client audio buffer empty");
    }
  } else if (audioDataSize) {
    // We actually have audio data!
    // We'll just assume float32 for now...
    float *rawValues = static_cast<float *>(malloc(nbytes));
    memcpy(rawValues, audioData, nbytes);

    for (int index = 0; index < audioDataSize / sizeof(float); index++) {
      if (rawValues[index] > 1)
        rawValues[index] = 1;
      if (rawValues[index] < -1)
        rawValues[index] = -1;
    }

    instance->newData(rawValues, audioDataSize / 4);

    free(rawValues);

    pa_stream_drop(p);
  } else {
    instance->logger.WriteLog(
        DebugLogger::DebugLevel::DEBUG_WARNING,
        "client audio unknown state! [buffer : %p] [size : %d]", audioData,
        audioDataSize);
  }
}