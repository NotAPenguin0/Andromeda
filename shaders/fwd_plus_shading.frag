#version 450

#extension GL_EXT_nonuniform_qualifier : enable

#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 1024
#define PI 3.14159265358979

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 world_pos;
layout(location = 2) in mat3 TBN;

layout(set = 0, binding = 0) uniform Camera {
	mat4 view;
	mat4 projection;
	mat4 projection_view;
	vec3 cam_pos;
} camera;

struct PointLight {
	// First 3 values have the position, last has the radius of the light
    vec4 transform;
    // first 3 values are the color, last value has the intensity of the light
    vec4 color;
};

layout(set = 0, binding = 2) buffer readonly Lights {
	PointLight data[];
} lights;

struct VisibleLightIndex {
	int index;
};

layout(set = 0, binding = 3) buffer readonly VisibleIndices {
	VisibleLightIndex data[];
} visible_light_indices;


layout(set = 0, binding = 4) uniform samplerCube irradiance_map;
layout(set = 0, binding = 5) uniform samplerCube specular_map;
layout(set = 0, binding = 6) uniform sampler2D brdf_lookup;

layout(set = 0, binding = 7) uniform sampler2D textures[];

layout(push_constant) uniform PC {
    uint transform_idx;
    uint albedo_idx;
    uint normal_idx;
    uint metallic_idx;
    uint roughness_idx;
    uint ambient_occlusion_idx;
    uint tile_count_x;
} pc;

float saturate(float x) {
    return max(0.0, min(x, 1.0));
}

float calculate_attenuation(PointLight light) {
    float dist = length(world_pos - light.transform.xyz);
    float radius = light.transform.w;
    float num = saturate(1 - pow(dist / radius, 4));
    return light.color.a * num * num / (dist * dist + 1);
}

float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

vec3 fresnel_schlick_roughness(float cos_theta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}   

float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometry_schlick_ggx(NdotV, roughness);
    float ggx1  = geometry_schlick_ggx(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 apply_point_light(PointLight light, vec3 norm, vec3 albedo, float roughness, float metallic) {
    vec3 light_dir = normalize(light.transform.xyz - world_pos);
    vec3 view_dir = normalize(camera.cam_pos - world_pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);

    vec3 radiance = light.color.rgb * calculate_attenuation(light);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnel_schlick(max(dot(halfway_dir, view_dir), 0.0), F0);

    // Calculate Cook-Torrance BRDF.
    float NDF = distribution_ggx(norm, halfway_dir, roughness);
    float G = geometry_smith(norm, view_dir, light_dir, roughness);

    vec3 num = NDF * G * F;
    float denom = 4.0 * max(dot(norm, view_dir), 0.0) * max(dot(norm, light_dir), 0.0);
    vec3 specular = num / max(denom, 0.001); // Make sure to not divide by zero 
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	

    float NdotL = max(dot(norm, light_dir), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 indirect_light(vec3 normal, vec3 albedo, float ao, float roughness, float metallic) {
	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, albedo, metallic);

	vec3 view_dir = normalize(camera.cam_pos - world_pos);

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

   	return (kD * diffuse + specular) * ao; 
}

void main() {
	// Figure out the tile we're on and its index, so we can index into the visible indices buffer correctly
    const ivec2 pixel_position = ivec2(gl_FragCoord.xy);
    const ivec2 tile_id = pixel_position / ivec2(TILE_SIZE, TILE_SIZE);
    const uint tile_index = tile_id.y * pc.tile_count_x + tile_id.x;

    // Retreive material data
    vec3 albedo = texture(textures[pc.albedo_idx], UV).rgb;
    vec3 normal = normalize(TBN * (texture(textures[pc.normal_idx], UV).rgb * 2.0f - 1.0f));
    float metallic = texture(textures[pc.metallic_idx], UV).r;
    float roughness = texture(textures[pc.roughness_idx], UV).r;
    float ambient_occlusion = texture(textures[pc.ambient_occlusion_idx], UV).r;

    vec3 final_color = vec3(0);

    const uint light_index_offset = MAX_LIGHTS_PER_TILE * tile_index;
    for(uint i = 0; (i < MAX_LIGHTS_PER_TILE) && (visible_light_indices.data[light_index_offset + i].index != -1); ++i) {
        const uint light_index = visible_light_indices.data[light_index_offset + i].index;
        PointLight light = lights.data[light_index];
        final_color += apply_point_light(light, normal, albedo, roughness, metallic);
    }

    // Apply indirect ambient lighting
    final_color += indirect_light(normal, albedo, ambient_occlusion, roughness, metallic);

    FragColor = vec4(final_color, 1.0);
}