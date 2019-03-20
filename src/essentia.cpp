#include "essentia.hpp"

NeonEssentia::NeonEssentia()
    : logger("Essentia", DebugLogger::DebugColor::COLOR_GREEN, true) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Created Essentia");
  essentia::init();
}

NeonEssentia::~NeonEssentia() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_STATUS, "Destroyed Essentia");
}
