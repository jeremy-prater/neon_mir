#pragma once

#include <Magnum/Math/Matrix4.h>

class NeonRenderable {
public:
  NeonRenderable(const std::string name);
  ~NeonRenderable();

  void render(double dTime);

private:
    Magnum::Math::Matrix4 transform;
};