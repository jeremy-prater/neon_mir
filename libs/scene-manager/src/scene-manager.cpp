#include "scene-manager.hpp"

SceneManager::SceneManager()
    : numSlices(0),
      logger("SceneManager", DebugLogger::DebugColor::COLOR_MAGENTA, true) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Created Scene Manager");
}

SceneManager::~SceneManager() {}

void SceneManager::updateSpectrumConfig(const std::string config) noexcept {
  frequencyEvents.clear();

  rapidjson::Document spectrumConfigDocument;
  spectrumConfigDocument.Parse(config.c_str());

  numSlices = spectrumConfigDocument["num_slices"].GetUint();
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Update Config - Num slices [%d]", numSlices);

  auto events = spectrumConfigDocument["events"].GetObject();

  for (rapidjson::Value::ConstMemberIterator iter = events.MemberBegin();
       iter != events.MemberEnd(); ++iter) {
    auto name = iter->name.GetString();
    auto event = iter->value.GetObject();

    auto min = event["min"].GetDouble();
    auto max = event["max"].GetDouble();
    auto threshold = event["threshold"].GetDouble();
    auto cooldown = event["cooldown"].GetDouble();

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Frequency Event [%s] [%f->%f] > %f @ %f ms", name, min, max,
                    threshold, cooldown);
    struct FrequencyEvent newEvent = {min, max, threshold, cooldown, name};
    frequencyEvents.push_back(newEvent);
  }
}
void SceneManager::updateSpectrumData(const float *data) {}
