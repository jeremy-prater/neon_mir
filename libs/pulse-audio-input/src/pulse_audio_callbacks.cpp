#include "pulse_audio_stream.hpp"
#include <pulse/pulseaudio.h>
#include <pulse/utf8.h>
#include <string.h>

void NeonPulseInput::serverInfoCallback(pa_context *c, const pa_server_info *i,
                                        void *userdata) {
  (void)c;
  (void)userdata;
  if (i != NULL) {
    NeonPulseInput *context = (NeonPulseInput *)userdata;
    // Display server info in debug log
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Pulse Audio Server Information");
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             " -- Username       : %s", i->user_name);
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             " -- Hostname       : %s", i->host_name);
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             " -- Server Version : %s", i->server_version);
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             " -- Server Name    : %s", i->server_name);
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             " -- Default Sink   : %s", i->default_sink_name);
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             " -- Default Source : %s", i->default_source_name);
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
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

  context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                           "Module [%d: %s %s] is loaded and %s", i->index,
                           i->name, i->argument,
                           i->n_used == PA_INVALID_INDEX ? "un-used" : "used");
}

void NeonPulseInput::stateChangeCallback(pa_context *c, void *userdata) {
  NeonPulseInput *context = (NeonPulseInput *)userdata;

  switch (UpdateCurrentState(c)) {
  case PA_CONTEXT_READY: {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Context state: PA_CONTEXT_READY");
  } break;
  case PA_CONTEXT_FAILED: {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Context state: PA_CONTEXT_FAILED");
  } break;
  case PA_CONTEXT_UNCONNECTED: {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Context state: PA_CONTEXT_UNCONNECTED");
  } break;
  case PA_CONTEXT_AUTHORIZING: {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Context state: PA_CONTEXT_AUTHORIZING");
  } break;
  case PA_CONTEXT_SETTING_NAME: {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Context state: PA_CONTEXT_SETTING_NAME");
  } break;
  case PA_CONTEXT_CONNECTING: {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Context state: PA_CONTEXT_CONNECTING");
  } break;
  case PA_CONTEXT_TERMINATED: {
    context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                             "Context state: PA_CONTEXT_TERMINATED");
  } break;
  }
}

void NeonPulseInput::eventCallback(pa_context *c,
                                   pa_subscription_event_type_t t,
                                   uint32_t index, void *userdata) {
  (void)c;
  NeonPulseInput *context = (NeonPulseInput *)userdata;
  context->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                           "Pulse Audio Event: [0x%08x] -> [%d]",
                           t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK, index);
}

void NeonPulseInput::successCallback(pa_context *context, int success,
                                     void *userdata) {
  (void)context;
  NeonPulseInput *neonContext = (NeonPulseInput *)userdata;

  if (!success) {
    neonContext->logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                                 "Pulse Audio Command Failed");
  }
}
