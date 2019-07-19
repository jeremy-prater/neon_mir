#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <string>
#include <vector>

class NeonRenderable {
public:
  NeonRenderable() {}
  ~NeonRenderable() {}

  virtual void render(double dTime) = 0;

private:
};

class NeonObject {
public:
  NeonObject() {}
  ~NeonObject() {}

  void addRenderable(NeonRenderable *newRenderable) {
    renderables.push_back(newRenderable);
  }

  void render(double dTime) {
    for (auto &renderable : renderables) {
      renderable->render(dTime);
    }
  }

private:
  std::vector<NeonRenderable *> renderables;
  Magnum::Matrix4 transform;
};
