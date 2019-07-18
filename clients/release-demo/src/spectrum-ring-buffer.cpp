#include "release-demo.hpp"

inline void NeonReleaseDemo::spectrumDataCheckSlice(uint32_t &position) const {
  if (position == RING_SIZE) {
    position = 0;
  }
}

inline bool NeonReleaseDemo::spectrumDataMeanEmpty() const noexcept {
  // Lock access to the circular buffer
  std::scoped_lock<std::mutex> lock(audioFrameMutex);
  bool result = (spectrumDataMeanTail == spectrumDataMeanHead);
  // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s %d %d = %d",
  //                 __func__, spectrumDataMeanHead, spectrumDataMeanTail,
  //                 result);
  return result;
}

float *NeonReleaseDemo::spectrumDataMeanGetSlice() const noexcept {
  // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s", __func__);

  // Attempt to move the tail up
  if (spectrumDataMeanEmpty()) {
    // Buffer Underrun
    // We're all caught up... No data this time...
    return nullptr;
  } else {
    // Return data and move up tail
    auto returnData = spectrumDataMean[spectrumDataMeanTail++];
    spectrumDataCheckSlice(spectrumDataMeanTail);
    return returnData;
  }
}
void NeonReleaseDemo::spectrumDataMeanFillSlice(uint32_t index, float data) {
  // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s", __func__);

  const float scale = 1.0f;
  spectrumDataMean[spectrumDataMeanHead][index] = data * scale;
}

void NeonReleaseDemo::spectrumDataMeanPushSlice() noexcept {
  // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s", __func__);

  // Lock access to the circular buffer
  std::scoped_lock<std::mutex> lock(audioFrameMutex);
  // Attempt to push data
  // Fix corner case here ...
  if (spectrumDataMeanHead + 1 == spectrumDataMeanTail) {
    // Buffer Overrun... Force client to lose data... Sorry
    spectrumDataMeanTail++;
    spectrumDataCheckSlice(spectrumDataMeanTail);
  } else {
    spectrumDataMeanHead++;
    spectrumDataCheckSlice(spectrumDataMeanHead);
  }
}