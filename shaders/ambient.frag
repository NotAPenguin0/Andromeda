#version 450

layout(location = 0) in vec2 UV;

layout(set = 0, binding = 0) uniform sampler2D gAlbedoAO;

layout(location = 0) out vec4 FragColor;

void main() {
	vec4 albedo_ao = texture(gAlbedoAO, UV);
	vec3 albedo = albedo_ao.rgb;
	float ao = albedo_ao.a;
	// Fixed ambient strength of 0.03
	vec3 ambient = vec3(0.02) * albedo * ao;
	FragColor = vec4(ambient, 1.0);
}