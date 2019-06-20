#include "test-full-pipe.hpp"
#include <Corrade/Utility/Resource.h>
#include <Magnum/Math/Complex.h>

NeonFullPipe::NeonFullPipe(const Arguments &arguments)
    : Platform::Application{arguments, Configuration{}.setTitle(
                                           "Neon MIR Graphics Test 1")},
      logger("RenderPipe", DebugLogger::DebugColor::COLOR_GREEN, false) {

  using namespace Magnum::Math::Literals;
  using namespace Corrade;

  Utility::Resource rs{"data"};

  /* Print out the license */
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Welcome !! ==> %s",
                  rs.get("motd").c_str());

  GL::Renderer::setClearColor(0xa5c9ea_rgbf);

  auto version = GL::Context::current().version();

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Booting on OpenGL %d using %s", version,
                  GL::Context::current().rendererString().c_str());

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
          dataConnection.disconnect();
          paInput.DestroyStream();
          paInput.Disconnect();
          io_service.stop();
        });

    io_service.run();
  });
}

NeonFullPipe::~NeonFullPipe() {
  if (audioWorker.joinable()) {
    audioWorker.join();
  }
}

void NeonFullPipe::initalizeRenderData() {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "%s", __func__);

  GL::Buffer buffer;

  spectrumData[0] = {{0.92f, -0.8f}, {1, 0}}; // Origin point

  const Vector2 start = {-0.9f, -0.75f};
  const Vector2 end = {0.5f, 0.9f};

  const float scale = RAND_MAX / 1.3; //170;

  for (int index = 0; index < NUM_SLICES; index++) {
    const float percent = static_cast<float>(index) / NUM_SLICES;
    auto newVector = Magnum::Math::lerp(start, end, percent);
    newVector[0] *= static_cast<float>(rand()) / scale;
    newVector[1] *= static_cast<float>(rand()) / scale;
    TriangleVertex newSlice = {{newVector}, {1.0f - percent, 1}};

    logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                    "new slice %d ==> %f ==> %f, %f", index, percent,
                    newSlice.position.x(), newSlice.position.y());
    spectrumData[index + 1] = newSlice;
  }

  // Super debug mode
  // for (int index = 0; index < 5; index++) {
  //   logger.WriteLog(
  //       DebugLogger::DebugLevel::DEBUG_INFO, "slice %d ==> %f, %f (%f, %f",
  //       index, spectrumData[index].position.x(),
  //       spectrumData[index].position.y(), spectrumData[index].textureUV.x(),
  //       spectrumData[index].textureUV.y());
  // }
  // logger.WriteLog(
  //     DebugLogger::DebugLevel::DEBUG_INFO, "slice 512 ==> %f, %f (%f, %f",
  //     spectrumData[512].position.x(), spectrumData[512].position.y(),
  //     spectrumData[512].textureUV.x(), spectrumData[512].textureUV.y());
  // spectrumData[index] = {{-0.9f, -0.35f}, {1, 1}}; // Bottom left
  // spectrumData[2] = {{0.5f, 0.9f}, {0, 1}};    // Top Right

  buffer.setData(spectrumData);
  _mesh.setPrimitive(Magnum::GL::MeshPrimitive::TriangleFan);
  _mesh.setCount(NUM_SLICES + 1)
      .addVertexBuffer(std::move(buffer), 0, Shaders::Vector2D::Position{},
                       Shaders::Vector2D::TextureCoordinates{});
}

void NeonFullPipe::drawEvent() {

  Debug{} << __func__;

  GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

  _mesh.draw(_shader);

  swapBuffers();
}

MAGNUM_APPLICATION_MAIN(NeonFullPipe)
