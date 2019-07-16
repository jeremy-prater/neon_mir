#pragma once

#include "debuglogger.hpp"
#include <mutex>
#include <string>

class SceneManager {
public:
  SceneManager();
  ~SceneManager();

private:
  DebugLogger logger;
};