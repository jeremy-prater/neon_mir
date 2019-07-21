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

NeonGrid1::NeonGrid1()
    : logger("NeonGrid1", DebugLogger::DebugColor::COLOR_CYAN, false) {
  addRenderable(&grid1);
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Created NeonGrid1");
  // transform = Magnum::Matrix4::translation(Magnum::Vector3::zAxis(-100)) *
  //             Magnum::Matrix4::translation(Magnum::Vector3::xAxis(size / 2))
  //             * Magnum::Matrix4::scaling(Vector3{size, size, size});
  // Magnum::Matrix4::rotationY(90.0_degf);
}
NeonGrid1::~NeonGrid1() {}

void NeonGrid1::render(double dTime) {
  // transform = transform * Magnum::Matrix4::rotationZ(5.0_degf);

  NeonObject::render(dTime);
}

struct TriangleVertex {
  Vector3 position;
  Vector2 textCoord;
};

const float planeSize = 1000.0f;
const float zDepth = -100;

const TriangleVertex data[]{{{planeSize, planeSize, zDepth}, {1.0f, 1.0f}},
                            {{0, planeSize, zDepth}, {0.0f, 1.0f}},
                            {{planeSize, -planeSize, zDepth}, {1.0f, 0.0f}},
                            {{0, -planeSize, zDepth}, {0.0f, 0.0f}}};

NeonGridRenderable1::NeonGridRenderable1()
    : logger("NeonGridRenderable1", DebugLogger::DebugColor::COLOR_CYAN,
             false) {
  MAGNUM_ASSERT_GL_VERSION_SUPPORTED(Magnum::GL::Version::GL330);
  const Magnum::Utility::Resource rs{"shaders"};

  rapidjson::Document gridConfigJson;
  auto jsonString = rs.get("grid1.json");
  gridConfigJson.Parse(jsonString.c_str());

  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "Created NeonGridRenderable1");

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
      .setCount(4)
      .addVertexBuffer(vertexBuffer, 0, GridShader1::Position{},
                       GridShader1::TextureCoordinates{});

  projection = NeonReleaseDemo::getInstance()->GetProjection();
}

NeonGridRenderable1::~NeonGridRenderable1() {}

void NeonGridRenderable1::render(double dTime) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "NeonGridRenderable1 - Render [%f]", dTime);

  shader.setViewProjectionMatrix(*projection)
      .setTransformationMatrix(*transform)
      .setBaseColor(baseColor)
      .setAccent1Color(accentColor1)
      .setAccent2Color(accentColor2)
      .setNumSlices(numSlices);

  mesh.draw(shader);
}