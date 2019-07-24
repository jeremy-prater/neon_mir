uniform vec3 baseColor;
uniform vec3 accent1Color;
uniform vec3 accent2Color;
uniform float spectrum[64];
uniform uint numSlices;
uniform float theta;

in vec2 interpolatedTextureCoordinates;

out vec4 fragmentColor;

void main() {
  vec4 outColor = vec4(0., 0., 0., 0.);
  vec4 spectrumColor;

  int index = int(interpolatedTextureCoordinates.x * numSlices);
  float value = spectrum[index];
  if (value >= 1 - interpolatedTextureCoordinates.y) {
    spectrumColor = vec4(mix(accent1Color, accent2Color,
                             pow(1 - interpolatedTextureCoordinates.y, 0.5)),
                         0);
  } else
    discard;

  int xScale = 1000;
  int xRatio = 25;
  int xThreshold = 10;

  int yScale = 1000;
  int yRatio = 50;
  int yThreshold = 25;

  if ((int(interpolatedTextureCoordinates.y * yScale) % yRatio >= yThreshold) &&
      (int(interpolatedTextureCoordinates.x * xScale) % xRatio >= xThreshold))
    outColor = spectrumColor;

  fragmentColor = outColor;
}