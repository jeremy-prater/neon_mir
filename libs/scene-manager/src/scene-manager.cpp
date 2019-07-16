#include "scene-manager.hpp"

SceneManager::SceneManager()
    : logger("SceneManager", DebugLogger::DebugColor::COLOR_MAGENTA, true) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Created Scene Manager");
}

SceneManager::~SceneManager() {}

void SceneManager::updateSpectrumConfig(const std::string config) noexcept {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Update Config[%s]",
                  config.c_str());

  frequencyEvents.clear();

  rapidjson::Document spectrumConfigDocument;
  spectrumConfigDocument.Parse(config.c_str());

  for (rapidjson::Value::ConstMemberIterator iter =
           spectrumConfigDocument.MemberBegin();
       iter != spectrumConfigDocument.MemberEnd(); ++iter) {
    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Frequency Event [%s]",
                    iter->name.GetString());
    // iter->value.GetString()
  }

  struct FrequencyEvent {
    double fMin;
    double fMax;
    double threshold;
    std::string fEventName;
  };
}
void SceneManager::updateSpectrumData(const float *data) {}
