uniform vec3 baseColor;
uniform vec3 accent1Color;
uniform vec3 accent2Color;
uniform uint numSlices;
uniform float theta;

uniform vec3 color = vec3(1., 1., 1.);
// uniform sampler2D textureData;

in vec2 interpolatedTextureCoordinates;

out vec4 fragmentColor;

void main() {
  vec4 outColor = vec4(0., 0., 0., 0.);

  float lineRate = .15;
  float widthScale = 10;

  float newX = interpolatedTextureCoordinates.x + theta;
  float newY = interpolatedTextureCoordinates.y;

  float xSlice =
      pow(abs(fract((newX * numSlices) / widthScale) - .5), lineRate);
  float ySlice =
      pow(abs(fract((newY * numSlices) / widthScale) - .5), lineRate);

  outColor = max(mix(vec4(accent1Color, 1.), outColor, ySlice),
                 mix(vec4(accent2Color, 1.), outColor, xSlice));

  fragmentColor = outColor;
}