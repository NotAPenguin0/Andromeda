#version 450

layout(location = 0) in vec3 iPos;

layout(location = 0) out vec3 UVW;

layout(set = 0, binding = 0) uniform Camera {
	mat4 model;
	mat4 projection;
} camera;

void main() {
    UVW = iPos;
    vec4 pos = camera.projection * camera.model * vec4(iPos, 1.0);
    gl_Position = pos;
}