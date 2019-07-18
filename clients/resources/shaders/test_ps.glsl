uniform vec3 color = vec3(1.0, 1.0, 1.0);
// uniform sampler2D textureData;

in vec2 interpolatedTextureCoordinates;

out vec4 fragmentColor;

void main() {
  fragmentColor.b = color.b;
  fragmentColor.r = color.r * interpolatedTextureCoordinates.x;
  fragmentColor.g = color.g * interpolatedTextureCoordinates.y;
  fragmentColor.a = 1.0;
}