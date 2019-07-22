#include "grid-shader-1.hpp"

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>

using namespace Magnum;

GridShader1::GridShader1() {
  MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

  const Utility::Resource rs{"shaders"};

  GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
  GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

  vert.addSource(rs.get("grid-1_vert"));
  frag.addSource(rs.get("grid-1_ps"));

  CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));

  attachShaders({vert, frag});

  CORRADE_INTERNAL_ASSERT_OUTPUT(link());

  viewProjectionMatrixUniform = uniformLocation("viewProjectionMatrix");
  transformationMatrixUniform = uniformLocation("transformationMatrix");
  baseColorUniform = uniformLocation("baseColor");
  accent1ColorUniform = uniformLocation("accent1Color");
  accent2ColorUniform = uniformLocation("accent2Color");
  numSlicesUniform = uniformLocation("numSlices");
  thetaUniform = uniformLocation("theta");
  sceneMoodUniform = uniformLocation("sceneMood");
}
