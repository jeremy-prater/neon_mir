#include "release-demo.hpp"
#include <Corrade/Utility/Resource.h>
#include <Magnum/Math/Complex.h>

// Codec : MP4
// Resolution : 1080p
// RGBA

NeonReleaseDemo *NeonReleaseDemo::instance = nullptr;

NeonReleaseDemo *NeonReleaseDemo::getInstance() { return instance; }

NeonReleaseDemo::NeonReleaseDemo(const Arguments &arguments)
    : Platform::Application{arguments,
                            Configuration{}.setTitle("Neon MIR Scene Test 1")},
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
  GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
  GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

  projection =
      Matrix4::perspectiveProjection(
          35.0_degf, Vector2{windowSize()}.aspectRatio(), 0.01f, 100.0f) *
      Matrix4::lookAt(Vector3{0, 0, 10}, Vector3{0, 0, 0}, Vector3{1, 0, 0})
          .invertedRigid();

  auto version = GL::Context::current().version();

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Booting on OpenGL %d using %s", version,
                  GL::Context::current().rendererString().c_str());

  sceneManager.updateSpectrumConfig(rs.get("test1").c_str());
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

NeonReleaseDemo::~NeonReleaseDemo() {
  shutdown = true;

  // Then Audio first...
  if (audioWorker.joinable()) {
    audioWorker.join();
  }
}

void NeonReleaseDemo::initalizeRenderData() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s", __func__);
  renderObjects["grid1"] = new NeonGrid1();
  drawEvent();
}

void NeonReleaseDemo::updateRenderData() { drawEvent(); }

Matrix4 *NeonReleaseDemo::GetProjection() { return &projection; }

void NeonReleaseDemo::drawEvent() {
  GL::defaultFramebuffer.clear(GL::FramebufferClear::Color |
                               GL::FramebufferClear::Depth);

  static auto lastTime = std::chrono::steady_clock::now();
  auto curTime = std::chrono::steady_clock::now();
  auto dTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(curTime - lastTime)
          .count();
  lastTime = curTime;

  for (auto object : renderObjects) {
    object.second->render(dTime);
  }

  swapBuffers();
}

MAGNUM_APPLICATION_MAIN(NeonReleaseDemo)
