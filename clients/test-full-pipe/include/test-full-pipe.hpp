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

#define NUM_SLICES 513

using namespace Magnum;

struct TriangleVertex {
  Vector2 position;
  Vector2 textureUV;
};

class NeonFullPipe : public Platform::Application {
public:
  explicit NeonFullPipe(const Arguments &arguments);
  virtual ~NeonFullPipe();

private:
  TriangleVertex spectrumData[NUM_SLICES + 1];

  void initalizeRenderData();
  void drawEvent() override;

  NeonPulseInput paInput;
  boost::asio::io_service io_service;
  AudioProcessor audioProcessor;

  GL::Mesh _mesh;
  NeonTestShader1 _shader;

  std::thread audioWorker;

  DebugLogger logger;
};
