#version 450

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec2 iTexCoords;

layout(location = 0) out vec2 UV;

layout(set = 0, binding = 0) buffer readonly Transforms {
	mat4 models[];
} transforms;

layout(set = 0, binding = 1) uniform Camera {
	mat4 projection_view;
} camera;

layout(push_constant) uniform PC {
	uint transform_idx;
	uint tex_idx;
} pc;

void main() {
	UV = iTexCoords;
	gl_Position = camera.projection_view * transforms.models[pc.transform_idx] * vec4(iPos, 1.0);
}