uniform vec3 baseColor;
uniform vec3 accent1Color;
uniform vec3 accent2Color;
uniform uint numSlices;

uniform vec3 color = vec3(1.0, 1.0, 1.0);
// uniform sampler2D textureData;

in vec2 interpolatedTextureCoordinates;

out vec4 fragmentColor;

void main() {
  vec4 outColor = vec4(0.0, 0.0, 0.0, 0.0);

  float rate = 0.15;

  float xSlice =
      pow(abs(fract(interpolatedTextureCoordinates.x * numSlices / 15) - 0.5),
          rate);
  float ySlice =
      pow(abs(fract(interpolatedTextureCoordinates.y * numSlices / 15) - 0.5),
          rate);

  outColor = max(mix(vec4(accent1Color, 1.0), outColor, ySlice),
                 mix(vec4(accent2Color, 1.0), outColor, xSlice));

  fragmentColor = outColor;
}