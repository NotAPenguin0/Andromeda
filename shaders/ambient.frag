#version 450

layout(location = 0) in vec2 UV;

layout(set = 0, binding = 0) uniform sampler2D gAlbedoAO;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 2) uniform sampler2D gMetallicRoughness;
layout(set = 0, binding = 3) uniform sampler2D gDepth;

layout(set = 0, binding = 4) uniform CameraData {
    mat4 projection_view;
    mat4 inverse_projection;
    mat4 inverse_view;
    vec3 position;
} camera;

layout(set = 0, binding = 5) uniform samplerCube irradiance_map;

layout(location = 0) out vec4 FragColor;


// adapted from https://stackoverflow.com/questions/32227283/getting-world-position-from-depth-buffer-value
// Note that we don't remap the depth value since the vk depth range goes from 0 to 1, not from -1 to 1 like in OpenGL
vec3 WorldPosFromDepth(float depth, vec2 TexCoords) {
    float z = depth;

    vec4 clipSpacePosition = vec4(TexCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = camera.inverse_projection * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = camera.inverse_view * viewSpacePosition;

    return worldSpacePosition.xyz;
}

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

void main() {
	vec4 albedo_ao = texture(gAlbedoAO, UV);
	vec3 albedo = albedo_ao.rgb;
	float ao = albedo_ao.a;
	vec3 normal = texture(gNormal, UV).rgb * 2.0f - 1.0f;
	vec2 metallic_roughness = texture(gMetallicRoughness, UV).rg;
	float metallic = metallic_roughness.r;
	float roughness = metallic_roughness.g;
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	float depth = texture(gDepth, UV).r;
	vec3 WorldPos = WorldPosFromDepth(depth, UV);
	vec3 view_dir = normalize(camera.position - WorldPos);
	vec3 kS = fresnel_schlick(max(dot(normal, view_dir), 0.0), F0);
	vec3 kD = 1.0 - kS;
	vec3 irradiance = texture(irradiance_map, normal).rgb;
	vec3 diffuse  = irradiance * albedo;
	vec3 ambient = (kD * diffuse) * ao; 
	FragColor = vec4(ambient, 1.0);
}