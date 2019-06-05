#include "audio-session.hpp"
#include "memory.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <stdlib.h>

std::mutex AudioSession::activeSessionMutex;
std::unordered_map<boost::uuids::uuid, std::shared_ptr<AudioSession>,
                   boost::hash<boost::uuids::uuid>>
    AudioSession::activeSessions;

std::mutex AudioSession::activePipelinesMutex;
std::unordered_map<boost::uuids::uuid, std::shared_ptr<NeonEssentiaSession>,
                   boost::hash<boost::uuids::uuid>>
    AudioSession::activePipelines;

AudioSession::AudioSession(const boost::uuids::uuid newUUID)
    : uuid(newUUID), sampleRate(0), channels(0), width(0), duration(0),
      logger("AudioSession-" + boost::uuids::to_string(uuid),
             DebugLogger::DebugColor::COLOR_BLUE, true) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Creating Audio Session");
}

AudioSession::~AudioSession() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Destoryed Session!");
}

void AudioSession::updateConfig(uint32_t newSampleRate, uint8_t newChannels,
                                uint8_t newWidth, double newDuration) {
  if ((newWidth != 16) && (newWidth != 24) && (newWidth != 32)) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Invalid bit-depth [%d]", newWidth);
    return;
  }
  if ((newSampleRate != 44100) && (newSampleRate != 48000)) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_WARNING,
                    "Invalid sample-rate [%d]", newSampleRate);
    return;
  }

  sampleRate = newSampleRate;
  channels = newChannels;
  width = newWidth;
  duration = newDuration / 1000;
  logger.WriteLog(
      DebugLogger::DebugLevel::DEBUG_STATUS,
      "Updating Config : SR [%d] Width [%d] Channels [%d] Duration [%f]",
      sampleRate, width, channels, duration);

  size_t bytes = channels * (width / 8) * sampleRate * duration;
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Allocating [%d] bytes", bytes);

  audioData = boost::circular_buffer<uint8_t>(bytes);

  // TODO: Extract this into something else later for dynamic pipelines
  // essentiaSession.createBPMPipeline(sampleRate, channels, width, duration);
}

boost::circular_buffer<uint8_t> *AudioSession::getAudioSink() {
  return &audioData;
}