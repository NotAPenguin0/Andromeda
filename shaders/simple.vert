#version 460

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec3 iTangent;
layout(location = 3) in vec2 iUV;

layout(location = 0) out vec2 UV;

layout(set = 0, binding = 0) uniform Camera {
	mat4 pv;
} camera;

layout(push_constant) uniform PushConstants {
	mat4 model;
} pc;

void main() {
	UV = iUV;
	gl_Position = camera.pv * pc.model * vec4(iPos, 1.0);
}