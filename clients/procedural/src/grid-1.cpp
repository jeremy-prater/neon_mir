#include "grid-1.hpp"

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <rapidjson/document.h>

NeonGrid1::NeonGrid1()
    : logger("NeonGrid1", DebugLogger::DebugColor::COLOR_CYAN, false) {
  addRenderable(&grid1);
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Created NeonGrid1");
}
NeonGrid1::~NeonGrid1() {}

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
}
NeonGridRenderable1::~NeonGridRenderable1() {}

void NeonGridRenderable1::render(double dTime) {
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO,
                  "NeonGridRenderable1 - Render [%f]", dTime);
}