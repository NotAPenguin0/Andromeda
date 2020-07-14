#version 450

layout(location = 0) in vec2 UV;

layout(set = 0, binding = 0) uniform sampler2DMS gAlbedoAO;
layout(set = 0, binding = 1) uniform sampler2DMS gNormal;
layout(set = 0, binding = 2) uniform sampler2DMS gMetallicRoughness;
layout(set = 0, binding = 3) uniform sampler2DMS gDepth;

layout(set = 0, binding = 4) uniform CameraData {
    mat4 projection_view;
    mat4 inverse_projection;
    mat4 inverse_view;
    vec3 position;
} camera;

layout(set = 0, binding = 5) uniform samplerCube irradiance_map;
layout(set = 0, binding = 6) uniform samplerCube specular_map;
layout(set = 0, binding = 7) uniform sampler2D brdf_lookup;

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

vec3 fresnel_schlick_roughness(float cos_theta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}   

void main() {
	ivec2 attachment_size = textureSize(gAlbedoAO);
	ivec2 texel_coords = ivec2(UV * attachment_size);

	const int SAMPLE_COUNT = 8;
	vec3 ambient = vec3(0);
	float depth_accum = 0;
	for (int i = 0; i < SAMPLE_COUNT; ++i) {
		vec4 albedo_ao = texelFetch(gAlbedoAO, texel_coords, i);
		vec3 albedo = albedo_ao.rgb;
	
		float ao = albedo_ao.a;
		vec3 normal = texelFetch(gNormal, texel_coords, i).rgb * 2.0f - 1.0f;
	
		vec2 metallic_roughness = texelFetch(gMetallicRoughness, texel_coords, i).rg;
		float metallic = metallic_roughness.r;
		float roughness = metallic_roughness.g;
	
		vec3 F0 = vec3(0.04); 
		F0 = mix(F0, albedo, metallic);

		float depth = texelFetch(gDepth, texel_coords, i).r;
		depth_accum += depth;
		vec3 WorldPos = WorldPosFromDepth(depth, UV);
		vec3 view_dir = normalize(camera.position - WorldPos);

		vec3 F = fresnel_schlick_roughness(max(dot(normal, view_dir), 0.0), F0, roughness);
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= (1.0 - metallic);
		vec3 irradiance = texture(irradiance_map, normal).rgb;
		vec3 diffuse = irradiance * albedo;
	
		vec3 reflect_dir = reflect(-view_dir, normal);
		const float MAX_REFLECTION_LOD = 7.0f; // Currently hardcoded, needs a better system
		float lod = MAX_REFLECTION_LOD * roughness;
		vec3 indirect_specular = textureLod(specular_map, reflect_dir, lod).rgb;
		vec2 env_brdf  = texture(brdf_lookup, vec2(max(dot(normal, view_dir), 0.0), roughness)).rg;
		vec3 specular = indirect_specular * (F * env_brdf.x + env_brdf.y);

		ambient += (kD * diffuse + specular) * ao; 
	}

	ambient = ambient / float(SAMPLE_COUNT);
	gl_FragDepth = depth_accum / float(SAMPLE_COUNT);
	FragColor = vec4(ambient, 1.0);
}