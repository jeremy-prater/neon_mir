#pragma once

#include "NeonObject.hpp"
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>

class NeonGridRenderable1 : public NeonRenderable {
public:
  NeonGridRenderable1();
  ~NeonGridRenderable1();
};

class NeonGrid1 : public NeonObject {
public:
private:
  GL::Buffer _indexBuffer, _vertexBuffer;
  GL::Mesh _mesh;
  Shaders::Phong _shader;

  Vector2i _previousMousePosition;
  Color3 _color;
};