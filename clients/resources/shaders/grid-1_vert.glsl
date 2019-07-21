uniform mat4 viewProjectionMatrix;
uniform mat4 transformationMatrix;

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 textureCoordinates;

out vec2 interpolatedTextureCoordinates;

void main(void) {
  interpolatedTextureCoordinates = textureCoordinates;

  gl_Position = viewProjectionMatrix * transformationMatrix * position;
}