#pragma once

#include "NeonObject.hpp"
#include "debuglogger.hpp"
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>

class NeonGridRenderable1 : public NeonRenderable {
public:
  NeonGridRenderable1();
  virtual ~NeonGridRenderable1();

  void render(double dTime) override;

private:
  Magnum::Color3 baseColor;
  Magnum::Color3 accentColor1;
  Magnum::Color3 accentColor2;
  uint32_t numSlices;

  DebugLogger logger;
};

class NeonGrid1 : public NeonObject {
public:
  NeonGrid1();
  ~NeonGrid1();

private:
  NeonGridRenderable1 grid1;
  DebugLogger logger;
};
