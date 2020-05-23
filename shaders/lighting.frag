#version 450

#define PI 3.12159265358979

layout(location = 0) flat in uint light_index;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 projection_view;
    mat4 inverse_projection;
    mat4 inverse_view;
    vec3 position;
} camera;

struct PointLight {
    // First 3 values have the position, last has the radius of the light
    vec4 transform;
    // first 3 values are the color, last value has the intensity of the light
    vec4 color;
};

layout(set = 0, binding = 1) buffer readonly PointLights {
   PointLight lights[];
} lights;


layout(set = 0, binding = 2) uniform sampler2D gDepth;
layout(set = 0, binding = 3) uniform sampler2D gNormal;
layout(set = 0, binding = 4) uniform sampler2D gAlbedoAO;
layout(set = 0, binding = 5) uniform sampler2D gMetallicRoughness;

layout(push_constant) uniform PC {
    uvec2 screen_size;
} pc;

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


float saturate(float x) {
    return max(0.0, min(x, 1.0));
}

float calculate_attenuation(vec3 WorldPos, PointLight light) {
    float dist = length(WorldPos - light.transform.xyz);
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

vec3 apply_point_light(vec3 norm, vec3 albedo, PointLight light, vec3 WorldPos, float roughness, float metallic) {
    vec3 light_dir = normalize(light.transform.xyz - WorldPos);
    vec3 view_dir = normalize(camera.position - WorldPos);
    vec3 halfway_dir = normalize(light_dir + view_dir);

    vec3 radiance = light.color.rgb * calculate_attenuation(WorldPos, light);

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

vec2 CalculateGBufferTexCoords(uvec2 screen_size) {
    return vec2(gl_FragCoord.x / float(screen_size.x), gl_FragCoord.y / float(screen_size.y));
}

void main() {             
    vec2 GBufferTexCoords = CalculateGBufferTexCoords(pc.screen_size);
    // retrieve data from gbuffer
    vec3 Normal = texture(gNormal, GBufferTexCoords).rgb;
    vec4 AlbedoAO = texture(gAlbedoAO, GBufferTexCoords);
    vec3 diffuse = AlbedoAO.rgb;
    vec2 MetallicRoughness = texture(gMetallicRoughness, GBufferTexCoords).rg;
    float metallic = MetallicRoughness.r;
    float roughness = MetallicRoughness.g;

    float depth = texture(gDepth, GBufferTexCoords).r;
    vec3 WorldPos = WorldPosFromDepth(depth, GBufferTexCoords);
    
    vec3 color = diffuse;
    vec3 norm = Normal * 2.0 - 1.0;
    
    color = apply_point_light(norm, color, lights.lights[light_index], WorldPos, roughness, metallic);

    FragColor = vec4(color, 1.0);
}

