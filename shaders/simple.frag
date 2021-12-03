#version 460

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform PushConstants {
	mat4 model;
	uint albedo;
} pc;

layout(set = 0, binding = 1) uniform sampler2D textures[];

void main() {
	FragColor = vec4(texture(textures[pc.albedo], UV).rgb, 1.0);
}