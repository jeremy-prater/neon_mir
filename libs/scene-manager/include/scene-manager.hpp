#pragma once

#include "debuglogger.hpp"
#include <boost/signals2.hpp>
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <string>
#include <unordered_map>
#include <vector>

struct FrequencyEvent {
  const uint16_t fMinBin;
  const uint16_t fMaxBin;
  const double threshold;
  const uint32_t cooldown;
  const std::string fEventName;
  int32_t cooldownTimer;
};

class SceneManager {
public:
  SceneManager();
  ~SceneManager();

  void updateSpectrumConfig(const std::string config) noexcept;
  void updateSpectrumData(const float *data);

  boost::signals2::signal<void(std::string name, const double frequency,
                               const double strength)>
      eventFired;

private:
  uint32_t numSlices;
  double logCoeff;
  double frequencyOffset;

  std::unordered_map<std::string, rapidjson::Value> spectrumConfig;

  std::vector<FrequencyEvent> frequencyEvents;
  std::mutex frequencyEventsMutex;

  DebugLogger logger;
};