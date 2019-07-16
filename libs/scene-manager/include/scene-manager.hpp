#pragma once

#include "debuglogger.hpp"
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <string>
#include <unordered_map>
#include <vector>

struct FrequencyEvent {
  double fMin;
  double fMax;
  double threshold;
  std::string fEventName;
};

class SceneManager {
public:
  SceneManager();
  ~SceneManager();

  void updateSpectrumConfig(const std::string config) noexcept;
  void updateSpectrumData(const float *data);

private:
  DebugLogger logger;
  std::unordered_map<std::string, rapidjson::Value> spectrumConfig;
  std::vector<FrequencyEvent> frequencyEvents;
};