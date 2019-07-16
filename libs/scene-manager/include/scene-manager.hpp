#pragma once

#include "debuglogger.hpp"
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <string>
#include <unordered_map>
#include <vector>

struct FrequencyEvent {
  const double fMin;
  const double fMax;
  const double threshold;
  const double cooldown;
  const std::string fEventName;
  double lastEvent;
};

class SceneManager {
public:
  SceneManager();
  ~SceneManager();

  void updateSpectrumConfig(const std::string config) noexcept;
  void updateSpectrumData(const float *data);

private:
  uint32_t numSlices;

  std::unordered_map<std::string, rapidjson::Value> spectrumConfig;
  std::vector<FrequencyEvent> frequencyEvents;

  DebugLogger logger;
};