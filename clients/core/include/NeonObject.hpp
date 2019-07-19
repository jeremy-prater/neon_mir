#pragma once

#include <Magnum/Math/Matrix4.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <string>
#include <vector>

class NeonRenderable {
public:
  NeonRenderable();
  ~NeonRenderable();

  void render(double dTime);

private:
  Magnum::GL::Shader vert{Magnum::GL::Version::GL330,
                          Magnum::GL::Shader::Type::Vertex};
  Magnum::GL::Shader frag{Magnum::GL::Version::GL330,
                          Magnum::GL::Shader::Type::Fragment};
};

class NeonObject {
public:
  NeonObject(const std::string name) {}
  ~NeonObject() {}

  void render(double dTime);

private:
  std::vector<NeonRenderable> renderables;
  Magnum::Math::Matrix4 transform;
};
