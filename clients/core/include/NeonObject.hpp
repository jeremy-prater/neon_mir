#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <string>
#include <vector>

using namespace Magnum::Math::Literals;

class NeonRenderable {
public:
  NeonRenderable() {}
  ~NeonRenderable() {}

  void SetTransform(Magnum::Matrix4 *newTransform) { transform = newTransform; }

  virtual void render(double dTime) = 0;

protected:
  Magnum::Matrix4 *transform;
  Magnum::Matrix4 *projection;
};

class NeonObject {
public:
  NeonObject() {
    transform = Magnum::Matrix4::translation(Magnum::Vector3::zAxis(-5.0f));
  }
  ~NeonObject() {}

  void addRenderable(NeonRenderable *newRenderable) {
    newRenderable->SetTransform(GetTransform());
    renderables.push_back(newRenderable);
  }

  void render(double dTime) {
    for (auto &renderable : renderables) {
      renderable->render(dTime);
    }
  }

  Magnum::Matrix4 *GetTransform() { return &transform; }

protected:
  std::vector<NeonRenderable *> renderables;
  Magnum::Matrix4 transform;
};
