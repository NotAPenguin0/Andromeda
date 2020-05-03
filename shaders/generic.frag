#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 UV;

layout(set = 0, binding = 2) uniform sampler2D textures[];

layout(push_constant) uniform PC {
	uint transform_idx;
	uint tex_idx;
} pc;

layout(location = 0) out vec4 FragColor;

void main() {
	FragColor = vec4(texture(textures[pc.tex_idx], UV).rgb, 1.0);
}