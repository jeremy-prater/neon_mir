#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>

using namespace Magnum;

class GridShader1 : public GL::AbstractShaderProgram {
public:
  typedef GL::Attribute<0, Vector3> Position;
  typedef GL::Attribute<1, Vector2> TextureCoordinates;

  explicit GridShader1();

  GridShader1 &setViewProjectionMatrix(const Matrix4 &vpMatrix) {
    setUniform(viewProjectionMatrixUniform, vpMatrix);
    return *this;
  }

  GridShader1 &setTransformationMatrix(const Matrix4 &tMatrix) {
    setUniform(transformationMatrixUniform, tMatrix);
    return *this;
  }

  GridShader1 &setBaseColor(const Vector3 &color) {
    setUniform(baseColorUniform, color);
    return *this;
  }

  GridShader1 &setAccent1Color(const Color3 &color) {
    setUniform(accent1ColorUniform, color);
    return *this;
  }

  GridShader1 &setAccent2Color(const Color3 &color) {
    setUniform(accent2ColorUniform, color);
    return *this;
  }

  GridShader1 &setNumSlices(const uint32_t &nSlice) {
    setUniform(numSlicesUniform, nSlice);
    return *this;
  }

  GridShader1 &setTheta(const float &theta) {
    setUniform(thetaUniform, theta);
    return *this;
  }

  GridShader1 &setsceneMood(const float &mood) {
    setUniform(sceneMoodUniform, mood);
    return *this;
  }

private:
  Int viewProjectionMatrixUniform, transformationMatrixUniform,
      baseColorUniform, accent1ColorUniform, accent2ColorUniform,
      numSlicesUniform, thetaUniform, sceneMoodUniform;
};