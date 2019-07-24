#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>

using namespace Magnum;

class MountainShader1 : public GL::AbstractShaderProgram {
public:
  typedef GL::Attribute<0, Vector3> Position;
  typedef GL::Attribute<1, Vector2> TextureCoordinates;

  explicit MountainShader1();

  MountainShader1 &setViewProjectionMatrix(const Matrix4 &vpMatrix) {
    setUniform(viewProjectionMatrixUniform, vpMatrix);
    return *this;
  }

  MountainShader1 &setTransformationMatrix(const Matrix4 &tMatrix) {
    setUniform(transformationMatrixUniform, tMatrix);
    return *this;
  }

  MountainShader1 &setBaseColor(const Vector3 &color) {
    setUniform(baseColorUniform, color);
    return *this;
  }

  MountainShader1 &setAccent1Color(const Color3 &color) {
    setUniform(accent1ColorUniform, color);
    return *this;
  }

  MountainShader1 &setAccent2Color(const Color3 &color) {
    setUniform(accent2ColorUniform, color);
    return *this;
  }

  MountainShader1 &setTheta(const float &theta) {
    setUniform(thetaUniform, theta);
    return *this;
  }

  MountainShader1 &setSpectrum(const Containers::ArrayView<const float> data, const uint32_t count) {
    setUniform(spectrumUniform, data);
    setUniform(numSlicesUniform, count);
    return *this;
  }

private:
  Int viewProjectionMatrixUniform, transformationMatrixUniform,
      baseColorUniform, accent1ColorUniform, accent2ColorUniform,
      numSlicesUniform, thetaUniform, spectrumUniform;
};