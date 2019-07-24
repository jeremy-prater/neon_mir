#pragma once

#include "NeonObject.hpp"
#include "debuglogger.hpp"
#include "mountain-shader-1.hpp"
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

class NeonMountainRenderable1 : public NeonRenderable {
public:
  NeonMountainRenderable1();
  virtual ~NeonMountainRenderable1();

  void updateSpectrum(const float *data, uint32_t count);
  void render(double dTime) override;

private:
  uint32_t numSlices;
  float spectrum[64];

  Magnum::Color3 baseColor;
  Magnum::Color3 accentColor1;
  Magnum::Color3 accentColor2;

  Magnum::GL::Buffer vertexBuffer;
  Magnum::GL::Mesh mesh;
  MountainShader1 shader;

  DebugLogger logger;
};

class NeonMountain1 : public NeonObject {
public:
  NeonMountain1();
  ~NeonMountain1();

  void updateSpectrum(const float *data, uint32_t count);
  virtual void render(double dTime) override;

private:
  NeonMountainRenderable1 mountain1;
  DebugLogger logger;
};
