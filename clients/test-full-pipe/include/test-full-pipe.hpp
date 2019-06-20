#include "audio-processor.hpp"
#include "debuglogger.hpp"
#include "pulse_audio_stream.hpp"
#include "test-shader-1.hpp"
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/Vector.h>
#include <Magnum/Shaders/VertexColor.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unistd.h>

using namespace Magnum;

class NeonGFX1 : public Platform::Application {
public:
  explicit NeonGFX1(const Arguments &arguments);
  virtual ~NeonGFX1();

private:
  void drawEvent() override;

  NeonPulseInput paInput;
  boost::asio::io_service io_service;
  AudioProcessor audioProcessor;

  GL::Mesh _mesh;
  NeonTestShader1 _shader;

  std::thread audioWorker;
};
