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
#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/Shaders/Vector.h>

using namespace Magnum;

class NeonGFX1 : public Platform::Application {
public:
  explicit NeonGFX1(const Arguments &arguments);

private:
  void drawEvent() override;

  GL::Mesh _mesh;
  NeonTestShader1 _shader;
};
