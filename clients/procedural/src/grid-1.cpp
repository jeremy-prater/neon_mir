#include "grid-1.hpp"

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>

NeonTestShader1::NeonTestShader1() {
  MAGNUM_ASSERT_GL_VERSION_SUPPORTED(Magnum::GL::Version::GL330);

  const Magnum::Utility::Resource rs{"shaders"};

  Magnum::GL::Shader vert{Magnum::GL::Version::GL330,
                          Magnum::GL::Shader::Type::Vertex};
  Magnum::GL::Shader frag{Magnum::GL::Version::GL330,
                          Magnum::GL::Shader::Type::Fragment};

  vert.addSource(rs.get("test_vs"));
  frag.addSource(rs.get("test_ps"));

  CORRADE_INTERNAL_ASSERT_OUTPUT(Magnum::GL::Shader::compile({vert, frag}));

  attachShaders({vert, frag});

  CORRADE_INTERNAL_ASSERT_OUTPUT(link());

  _colorUniform = uniformLocation("color");

  setUniform(uniformLocation("textureData"), TextureLayer);
}