#include "grid-1.hpp"

#include "release-demo.hpp"
#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Magnum.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Trade/MeshData3D.h>
#include <rapidjson/document.h>

using namespace Magnum::Math::Literals;

NeonMountain1::NeonMountain1()
    : logger("NeonMountain1", DebugLogger::DebugColor::COLOR_CYAN, false) {
  addRenderable(&mountain1);
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Created Mountain1");
  // transform = Magnum::Matrix4::translation(Magnum::Vector3::zAxis(-100)) *
  //             Magnum::Matrix4::translation(Magnum::Vector3::xAxis(size / 2))
  //             * Magnum::Matrix4::scaling(Vector3{size, size, size});
  // Magnum::Matrix4::rotationY(90.0_degf);
}
NeonMountain1::~NeonMountain1() {}

void NeonMountain1::render(double dTime) {
  // transform = transform * Magnum::Matrix4::rotationZ(5.0_degf);

  NeonObject::render(dTime);
}

void NeonMountain1::updateSpectrum(const float *data, uint32_t count) {
  mountain1.updateSpectrum(data, count);
}

struct TriangleVertex {
  Vector3 position;
  Vector2 textCoord;
};

const float planeSize = 1000.0f;
const float zDepth = -100.0f;

const TriangleVertex data[]{
    {{planeSize, planeSize * 0.1f, zDepth}, {1.0f, 1.0f}},
    {{planeSize, planeSize * 0.1f, -zDepth * 2}, {1.0f, 0.0f}},
    {{0.0f, planeSize, zDepth}, {0.0f, 1.0f}},
    {{0.0f, planeSize, -zDepth * 2}, {0.0f, 0.0f}},

    {{planeSize, -planeSize * 0.1f, zDepth}, {1.0f, 1.0f}},
    {{0.0f, -planeSize, zDepth}, {0.0f, 1.0f}},
    {{planeSize, -planeSize * 0.1f, -zDepth * 2}, {1.0f, 0.0f}},
    {{0.0f, -planeSize, -zDepth * 2}, {0.0f, 0.0f}}};

NeonMountainRenderable1::NeonMountainRenderable1()
    : logger("NeonMountainRenderable1", DebugLogger::DebugColor::COLOR_CYAN,
             false) {
  MAGNUM_ASSERT_GL_VERSION_SUPPORTED(Magnum::GL::Version::GL330);
  const Magnum::Utility::Resource rs{"shaders"};

  rapidjson::Document gridConfigJson;
  auto jsonString = rs.get("mountain-1_config");
  gridConfigJson.Parse(jsonString.c_str());

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Created NeonMountainRenderable1");

  auto baseColorJson = gridConfigJson["baseColor"].GetObject();
  baseColor = Magnum::Color3(baseColorJson["r"].GetDouble(),
                             baseColorJson["g"].GetDouble(),
                             baseColorJson["b"].GetDouble());

  auto accentColor1Json = gridConfigJson["accentColor1"].GetObject();
  accentColor1 = Magnum::Color3(accentColor1Json["r"].GetDouble(),
                                accentColor1Json["g"].GetDouble(),
                                accentColor1Json["b"].GetDouble());

  auto accentColor2Json = gridConfigJson["accentColor2"].GetObject();
  accentColor2 = Magnum::Color3(accentColor2Json["r"].GetDouble(),
                                accentColor2Json["g"].GetDouble(),
                                accentColor2Json["b"].GetDouble());

  numSlices = gridConfigJson["numSlices"].GetUint();

  vertexBuffer.setData(data);
  // Magnum::MeshTools::interleave(planeData.positions(0),
  // planeData.textureCoords2D(0)));

  mesh.setPrimitive(Magnum::GL::MeshPrimitive::TriangleStrip)
      .setCount(8)
      .addVertexBuffer(vertexBuffer, 0, GridShader1::Position{},
                       GridShader1::TextureCoordinates{});

  projection = NeonReleaseDemo::getInstance()->GetProjection();

  numSlices = 64;
  for (int index = 0; index < numSlices; index++) {
    spectrum[index] = static_cast<float>(rand()) / RAND_MAX;
  }
}

NeonMountainRenderable1::~NeonMountainRenderable1() {}

void NeonMountainRenderable1::updateSpectrum(const float *data,
                                             uint32_t count) {
  numSlices = count;
  for (int index = 0; index < numSlices; index++) {
    spectrum[index] = data[index] * 40;
    if (spectrum[index] > 1) {
      spectrum[index] = 1;
    }
  }
}

void NeonMountainRenderable1::render(double dTime) {
  static double theta = 0;

  const double scale = 0.1;
  dTime *= scale;

  theta += dTime;
  while (theta > 1000) {
    theta -= 1000;
  }

  const float decay = 1.0f - (dTime / 100);

  for (int index = 0; index < numSlices; index++) {
    spectrum[index] *= decay;
  }

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "NeonMountainRenderable1 - Render [%f]", dTime);

  shader.setViewProjectionMatrix(*projection)
      .setTransformationMatrix(*transform)
      .setBaseColor(baseColor)
      .setAccent1Color(accentColor1)
      .setAccent2Color(accentColor2)
      .setTheta(theta / 1000)
      .setSpectrum(spectrum, numSlices);

  mesh.draw(shader);
}