#pragma once

#include "NeonObject.hpp"
#include "debuglogger.hpp"
#include "grid-shader-1.hpp"
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Trade/MeshData3D.h>

class NeonGridRenderable1 : public NeonRenderable {
public:
  NeonGridRenderable1();
  virtual ~NeonGridRenderable1();

  void baseHit();
  void render(double dTime) override;

private:
  float dMode;

  Magnum::Color3 baseColor;
  Magnum::Color3 accentColor1;
  Magnum::Color3 accentColor2;
  uint32_t numSlices;

  Magnum::GL::Buffer vertexBuffer;
  Magnum::GL::Mesh mesh;
  GridShader1 shader;

  DebugLogger logger;
};

class NeonGrid1 : public NeonObject {
public:
  NeonGrid1();
  ~NeonGrid1();

  virtual void render(double dTime) override;
  void baseHit() { grid1.baseHit(); }

private:
  NeonGridRenderable1 grid1;
  DebugLogger logger;
};
