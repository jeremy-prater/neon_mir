#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>

class NeonTestShader1 : public Magnum::GL::AbstractShaderProgram {
public:
  typedef Magnum::GL::Attribute<0, Magnum::Vector2> Position;
  typedef Magnum::GL::Attribute<1, Magnum::Vector2> TextureCoordinates;

  explicit NeonTestShader1();

  NeonTestShader1 &setColor(const Magnum::Color3 &color) {
    setUniform(_colorUniform, color);
    return *this;
  }

  NeonTestShader1 &bindTexture(Magnum::GL::Texture2D &texture) {
    texture.bind(TextureLayer);
    return *this;
  }

private:
  enum : Magnum::Int { TextureLayer = 0 };

  Magnum::Int _colorUniform;
};