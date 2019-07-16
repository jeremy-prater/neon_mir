#include "test-scene-manager.hpp"
#include <Corrade/Utility/Resource.h>
#include <Magnum/Math/Complex.h>

// Codec : MP4
// Resolution : 1080p
// RGBA

NeonSpectrumGFX *NeonSpectrumGFX::instance = nullptr;

NeonSpectrumGFX *NeonSpectrumGFX::getInstance() { return instance; }

NeonSpectrumGFX::NeonSpectrumGFX(const Arguments &arguments)
    : Platform::Application{arguments, Configuration{}.setTitle(
                                           "Neon MIR Graphics Test 1")},
      spectrumDataMeanHead(0), spectrumDataMeanTail(RING_SIZE - 1),
      shutdown(false),
      logger("RenderPipe", DebugLogger::DebugColor::COLOR_GREEN, false) {

  using namespace Magnum::Math::Literals;
  using namespace Corrade;

  instance = this;

  Utility::Resource rs{"data"};

  /* Print out the license */
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Welcome !! ==> %s",
                  rs.get("motd").c_str());

  GL::Renderer::setClearColor(0xa5c9ea_rgbf);

  auto version = GL::Context::current().version();

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Booting on OpenGL %d using %s", version,
                  GL::Context::current().rendererString().c_str());

  sceneManager.updateSpectrumConfig(
      "{\"bass hit\":{\"min\":300,\"max\":500,\"threshold\":1},\"mid "
      "hit\":{\"min\":1000,\"max\":2000,\"threshold\":1},\"high "
      "hit\":{\"min\":3500,\"max\":5000,\"threshold\":1}}");
  initalizeRenderData();

  audioWorker = std::thread([this] {
    paInput.Connect();

    auto dataConnection = paInput.newData.connect(
        boost::bind(&AudioProcessor::processAudio, &audioProcessor, _1, _2));

    paInput.CreateStream(audioProcessor.getChannels(),
                         audioProcessor.getSampleRate(), PA_SAMPLE_FLOAT32LE);

    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait(
        [this, &dataConnection](const boost::system::error_code &error,
                                int signal_number) {
          (void)error;
          (void)signal_number;

          dataConnection.disconnect();
          paInput.DestroyStream();
          paInput.Disconnect();
          io_service.stop();
        });

    io_service.run();
  });

  // This is it... The main loop
  while (!shutdown) {
    // Just for fun...
    updateRenderData();

    usleep(50 * 1000); // 20 FPS @ 50 ms/frame
  }
}

NeonSpectrumGFX::~NeonSpectrumGFX() {
  shutdown = true;

  // Then Audio first...
  if (audioWorker.joinable()) {
    audioWorker.join();
  }
}

void NeonSpectrumGFX::initalizeRenderData() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s", __func__);

  spectrumData[0] = {{0.92f, -0.8f}, {1, 0}}; // Origin point

  const Vector2 start = {-0.9f, -0.75f};
  const Vector2 end = {0.5f, 0.9f};

  const float scale = RAND_MAX / 1.3; // 170;

  for (int index = 0; index < NUM_SLICES; index++) {
    float lastValue = 1;
    const float percent = static_cast<float>(index) / NUM_SLICES;
    auto newVector = Magnum::Math::lerp(start, end, percent);
    float randomAudioValue = static_cast<float>(rand()) / scale;
    newVector[0] *= lastValue;
    newVector[1] *= randomAudioValue;

    lastValue = -randomAudioValue;
    TriangleVertex newSlice = {{newVector}, {1.0f - percent, 1}};

    // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
    //                 "new slice %d ==> %f ==> %f, %f", index, percent,
    //                 newSlice.position.x(), newSlice.position.y());
    spectrumData[index + 1] = newSlice;
  }

  updateData();
  drawEvent();
}
inline void NeonSpectrumGFX::spectrumDataCheckSlice(uint32_t &position) const {
  if (position == RING_SIZE) {
    position = 0;
  }
}

inline bool NeonSpectrumGFX::spectrumDataMeanEmpty() const noexcept {
  // Lock access to the circular buffer
  std::scoped_lock<std::mutex> lock(audioFrameMutex);
  bool result = (spectrumDataMeanTail == spectrumDataMeanHead);
  // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s %d %d = %d",
  //                 __func__, spectrumDataMeanHead, spectrumDataMeanTail,
  //                 result);
  return result;
}

float *NeonSpectrumGFX::spectrumDataMeanGetSlice() const noexcept {
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
void NeonSpectrumGFX::spectrumDataMeanFillSlice(uint32_t index, float data) {
  // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s", __func__);

  const float scale = 1.0f;
  spectrumDataMean[spectrumDataMeanHead][index] = data * scale;
}

void NeonSpectrumGFX::spectrumDataMeanPushSlice() noexcept {
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

void NeonSpectrumGFX::updateRenderData() {
  // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s", __func__);

  // audioFrames should be a vector of frames...
  // We'll pop one from the stack and render it into the triangle...

  if (!spectrumDataMeanEmpty()) {

    // Implement a performance timer here...

    auto audioData = spectrumDataMeanGetSlice();

    if (audioData == nullptr)
      return;

    spectrumData[0] = {{0.92f, -0.8f}, {1, 0}}; // Origin point

    const Vector2 start = {-0.9f, -0.75f};
    const Vector2 end = {0.5f, 0.9f};

    const float scale = 5;

    for (int index = 0; index < NUM_SLICES; index++) {
      float lastValue = 1;
      const float percent = static_cast<float>(index) / NUM_SLICES;
      auto newVector = Magnum::Math::lerp(start, end, percent);

      // Convert from scale ...
      // -1.0 --- 0.0 --- 1.0
      //  -x  --- 1.0 --- +x

      float rawValue = audioData[index] * scale;

      float fuzz = 1.0f; //(static_cast<float>(rand()) / RAND_MAX) * (1.01);
      float audioValue = rawValue * fuzz;
      newVector[0] *= lastValue;
      newVector[1] *= audioValue;

      lastValue = -audioValue;
      TriangleVertex newSlice = {{newVector}, {1.0f - percent, 1}};

      // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
      //                 "new slice %d ==> %f ==> %f, %f", index, percent,
      //                 newSlice.position.x(), newSlice.position.y());
      spectrumData[index + 1] = newSlice;
    }
  } else {
    // TODO : Report no new audio data...
  }
  updateData();
  drawEvent();
}

void NeonSpectrumGFX::updateData() {
  // logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s", __func__);

  buffer.setData(spectrumData);
  _mesh.setPrimitive(Magnum::GL::MeshPrimitive::TriangleFan);
  _mesh.setCount(NUM_SLICES + 1)
      .addVertexBuffer(buffer, 0, Shaders::Vector2D::Position{},
                       Shaders::Vector2D::TextureCoordinates{});
}

void NeonSpectrumGFX::addSlices(std::vector<float> &newData) noexcept {}

void NeonSpectrumGFX::drawEvent() {

  // Debug{} << __func__;

  GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

  _mesh.draw(_shader);

  swapBuffers();
}

MAGNUM_APPLICATION_MAIN(NeonSpectrumGFX)
