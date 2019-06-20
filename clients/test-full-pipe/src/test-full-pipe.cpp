#include "test-full-pipe.hpp"
#include <Corrade/Utility/Resource.h>

NeonGFX1::NeonGFX1(const Arguments &arguments)
    : Platform::Application{
          arguments, Configuration{}.setTitle("Neon MIR Graphics Test 1")} {

  using namespace Magnum::Math::Literals;
  using namespace Corrade;

  Utility::Resource rs{"data"};

  /* Print out the license */
  Utility::Debug{} << rs.get("motd");

  GL::Renderer::setClearColor(0xa5c9ea_rgbf);

  Debug{} << "This application is running on"
          << GL::Context::current().version() << "using"
          << GL::Context::current().rendererString();

  struct TriangleVertex {
    Vector2 position;
    Vector2 textureUV;
  };

  // This needs to be 513 indexes long
  TriangleVertex data[]{
      {{-0.8f, -0.85f}, {1, 1}}, /* Left vertex, red color */
      {{0.75f, -0.5f}, {1, 0}},  /* Right vertex, green color */
      {{0.2f, 0.8f}, {0, 1}}     /* Top vertex, blue color */
  };

  // TODO : need buffer update function from the backend

  GL::Buffer buffer;
  buffer.setData(data);
  _mesh.setCount(513).addVertexBuffer(std::move(buffer), 0,
                                      Shaders::Vector2D::Position{},
                                      Shaders::Vector2D::TextureCoordinates{});

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

NeonGFX1::~NeonGFX1() {
  if (audioWorker.joinable()) {
    audioWorker.join();
  }
}

void NeonGFX1::drawEvent() {

  Debug{} << __func__;

  GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

  _mesh.draw(_shader);

  swapBuffers();
}

MAGNUM_APPLICATION_MAIN(NeonGFX1)
