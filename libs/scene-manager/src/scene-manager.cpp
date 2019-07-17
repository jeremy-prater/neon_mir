#include "scene-manager.hpp"
#include <chrono>
#include <cmath>

SceneManager::SceneManager()
    : numSlices(0), logCoeff(0), frequencyOffset(0),
      logger("SceneManager", DebugLogger::DebugColor::COLOR_MAGENTA, true) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Created Scene Manager");
}

SceneManager::~SceneManager() {}

void SceneManager::updateSpectrumConfig(const std::string config) noexcept {
  std::scoped_lock<std::mutex> lock(frequencyEventsMutex);
  frequencyEvents.clear();

  rapidjson::Document spectrumConfigDocument;
  spectrumConfigDocument.Parse(config.c_str());

  numSlices = spectrumConfigDocument["numSlices"].GetUint();
  logCoeff = spectrumConfigDocument["logCoeff"].GetDouble();
  frequencyOffset = spectrumConfigDocument["frequencyOffset"].GetDouble();
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Update Config - Num slices [%d]", numSlices);

  auto events = spectrumConfigDocument["events"].GetObject();

  for (rapidjson::Value::ConstMemberIterator iter = events.MemberBegin();
       iter != events.MemberEnd(); ++iter) {
    auto name = iter->name.GetString();
    auto event = iter->value.GetObject();

    auto min = static_cast<uint16_t>(
        log10(event["min"].GetDouble() - frequencyOffset) * logCoeff);
    auto max = static_cast<uint16_t>(
        log10(event["max"].GetDouble() - frequencyOffset) * logCoeff);
    auto threshold = event["threshold"].GetDouble();
    auto cooldown = event["cooldown"].GetUint();

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "Frequency Event [%s] [%d->%d] > %f @ %d ms", name, min,
                    max, threshold, cooldown);
    struct FrequencyEvent newEvent = {min, max, threshold, cooldown, name, 0};
    frequencyEvents.push_back(newEvent);
  }
}

void SceneManager::updateSpectrumData(const float *data) {
  // Do we iterate through events/slices, or through slices/events?
  std::scoped_lock<std::mutex> lock(frequencyEventsMutex);

  static auto lastTime = std::chrono::steady_clock::now();
  auto curTime = std::chrono::steady_clock::now();
  auto dTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(curTime - lastTime)
          .count();
  lastTime = curTime;

  for (auto &event : frequencyEvents) {
    if (event.cooldownTimer > 0) {
      event.cooldownTimer -= dTime;
      if (event.cooldownTimer <= 0) {
        event.cooldownTimer = 0;
        // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
        //                 "Frequency Event Cooled down [%s]",
        //                 event.fEventName.c_str());
      }
    }

    int maxBin = -1;
    double maxValue = 0;
    for (uint32_t currentBin = event.fMinBin; currentBin <= event.fMaxBin;
         currentBin++) {
      if ((data[currentBin] >= event.threshold) && (event.cooldownTimer == 0)) {
        if (data[currentBin] > maxValue) {
          maxBin = currentBin;
          maxValue = data[currentBin];
        }
        auto cFreq =
            pow(10, static_cast<double>(maxBin) / logCoeff) + frequencyOffset;
        auto fFreq = pow(10, static_cast<double>(currentBin) / logCoeff) +
                     frequencyOffset;
        logger.WriteLog(
            DebugLogger::DebugLevel::DEBUG_INFO,
            "Frequency Max Value [%s] [%d] [%f] [%f] Cur Value [%d] [%f] [%f]",
            event.fEventName.c_str(), maxBin, cFreq, data[maxBin], currentBin,
            fFreq, data[currentBin]);
      }
    }

    if (maxBin > 0) {
      event.cooldownTimer = event.cooldown;
      auto fFreq =
          pow(10, static_cast<double>(maxBin) / logCoeff) + frequencyOffset;

      logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                      "Frequency Event Fired [%s] [%f] > [%f] @ [%f]",
                      event.fEventName.c_str(), data[maxBin], event.threshold,
                      fFreq);

      eventFired(event.fEventName, fFreq, data[maxBin]);
    }
  }
}
