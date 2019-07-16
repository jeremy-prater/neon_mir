#include "scene-manager.hpp"

SceneManager::SceneManager()
    : logger("SceneManager", DebugLogger::DebugColor::COLOR_MAGENTA, true) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS,
                  "Created Scene Manager");
}

SceneManager::~SceneManager() {}