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

  void render(double dTime) = 0;

private:
};

class NeonObject {
public:
  NeonObject() {}
  ~NeonObject() {}

  void addRenderable();

  void render(double dTime) {
    for (auto &renderable : renderables) {
      renderable.render(dTime);
    }
  }

private:
  std::vector<NeonRenderable> renderables;
  Magnum::Math::Matrix4 transform;
};
