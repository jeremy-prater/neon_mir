uniform vec3 baseColor;
uniform vec3 accent1Color;
uniform vec3 accent2Color;
uniform uint numSlices;
uniform float theta;
uniform float sceneMood;

in vec2 interpolatedTextureCoordinates;

out vec4 fragmentColor;

void main() {
  vec4 outColor = vec4(0., 0., 0., 0.);

  float lineRate = mix(0.25, // V1 - Max
                             // 0.10,  // V2 - Avg
                       1.20, // V3 - Min
                       sceneMood);

  float widthScale = mix(10, // V1,V2 - Max,Avg
                         30, // V3 - Min
                         sceneMood);

  float newX = interpolatedTextureCoordinates.x + theta;
  float newY = interpolatedTextureCoordinates.y;

  float xSlice =
      pow(abs(fract((newX * numSlices) / widthScale) - .5), lineRate);
  float ySlice =
      pow(abs(fract((newY * numSlices) / widthScale) - .5), lineRate);

  // V1 - Max
  outColor = max(mix(vec4(accent1Color, 1.), outColor, ySlice),
                 mix(vec4(accent2Color, 1.), outColor, xSlice));

  // V2 - Avg
  // outColor = ((mix(vec4(accent1Color, 1.), outColor, ySlice) +
  //              mix(vec4(accent2Color, 1.), outColor, xSlice)) /
  //             2);

  // V3 - Min
  // outColor = min(mix(vec4(accent1Color, 1.), outColor, 1 - ySlice),
  //                mix(vec4(accent2Color, 1.), outColor, 1 - xSlice));

  // Negative space
  // if (outColor.a < .25) { // V1 - Max
  if (outColor.a < .17) { // V2 - Avg
                          // if (outColor.a < .17) { // V3 - Min
    discard;
  }

  fragmentColor = outColor;
}