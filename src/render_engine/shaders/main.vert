#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 projection;
} mvp;

layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;

layout(location = 0) out vec3 frag_color;

void main() {
  gl_Position = mvp.projection * mvp.view * mvp.model * vec4(position, 0.0, 1.0);
  frag_color = color;
}