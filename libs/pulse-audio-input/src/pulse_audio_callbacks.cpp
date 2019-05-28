#include "pulse_audio_stream.h"
#include <pulse/pulseaudio.h>
#include <pulse/utf8.h>
#include <string.h>

void TTSPulseAudio::serverInfoCallback(pa_context *c, const pa_server_info *i,
                                       void *userdata) {
  (void)c;
  (void)userdata;
  if (i != NULL) {
    // Display server info in debug log
    // qDebug() << QString("Pulse Audio Server Information");
    // qDebug() << QString(" -- Username       : %1").arg(i->user_name);
    // qDebug() << QString(" -- Hostname       : %1").arg(i->host_name);
    // qDebug() << QString(" -- Server Version : %1").arg(i->server_version);
    // qDebug() << QString(" -- Server Name    : %1").arg(i->server_name);
    // qDebug() << QString(" -- Default Sink   : %1").arg(i->default_sink_name);
    // qDebug() << QString(" -- Default Source : %1").arg(i->default_source_name);
    // qDebug() << QString(" -- Cookie         : 0x%1").arg(i->cookie, 0, 16);
  }
}

void TTSPulseAudio::moduleInfoCallback(pa_context *c, const pa_module_info *i,
                                       int eol, void *userdata) {
  (void)c;

  if (eol != 0) {
    return;
  }

  TTSPulseAudio *parent = (TTSPulseAudio *)userdata;

  QString moduleName = QString(i->name);
  if (moduleName.compare("module-pipe-source") == 0) {
        qDebug() <<  QString("Adding module-pipe-source [%1] for deletion!").arg(i->index);
        parent->moduleTrashList.append(i->index);
  } else if (moduleName.compare("module-loopback") == 0) {
    if (QString(i->argument).contains("media.name=polaris_carplay")) {
            qDebug() <<  QString("Adding module-loopback [%1] for deletion!").arg(i->index);
            parent->moduleTrashList.append(i->index);
    }
  }
}

void TTSPulseAudio::stateChangeCallback(pa_context *c, void *userdata) {
  (void)userdata;
  switch (UpdateCurrentState(c)) {
  case PA_CONTEXT_READY: {
    // qDebug() << QString("Context state: PA_CONTEXT_READY");
  } break;
  case PA_CONTEXT_FAILED: {
    // qDebug() << QString("Context state: PA_CONTEXT_FAILED");
  } break;
  case PA_CONTEXT_UNCONNECTED: {
    // qDebug() << QString("Context state: PA_CONTEXT_UNCONNECTED");
  } break;
  case PA_CONTEXT_AUTHORIZING: {
    // qDebug() << QString("Context state: PA_CONTEXT_AUTHORIZING");
  } break;
  case PA_CONTEXT_SETTING_NAME: {
    // qDebug() << QString("Context state: PA_CONTEXT_SETTING_NAME");
  } break;
  case PA_CONTEXT_CONNECTING: {
    // qDebug() << QString("Context state: PA_CONTEXT_CONNECTING");
  } break;
  case PA_CONTEXT_TERMINATED: {
    // qDebug() << QString("Context state: PA_CONTEXT_TERMINATED");
  } break;
  }
}

void TTSPulseAudio::eventCallback(pa_context *c, pa_subscription_event_type_t t,
                                  uint32_t index, void *userdata) {
  (void)c;
  (void)userdata;
    qDebug() <<  QString("Pulse Audio Event: [0x%1] -> [%2]").arg(t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK).arg(index);
}

void TTSPulseAudio::successCallback(pa_context *context, int success,
                                    void *userdata) {
  (void)context;
  (void)userdata;
  if (!success) {
    qDebug() << "Pulse Audio Command Failed";
  }
}
