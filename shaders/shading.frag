#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#include "include/glsl/inputs.glsl"
#include "include/glsl/types.glsl"
#include "include/glsl/limits.glsl"
#include "include/glsl/common.glsl"

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 world_pos;
layout(location = 2) in vec3 viewspace_pos;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec4 FragColor;

layout(std430, set = 0, binding = 2) buffer readonly Lights {
    PointLight l[];
} lights;

layout(std430, set = 0, binding = 3) buffer readonly DirectionalLights {
    DirectionalLight l[];
} dir_lights;


layout(scalar, set = 0, binding = 4) uniform CascadeMapInfos {
    CascadeMapInfo maps[ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS];
} cascade_map_info;


// These lights index into the lights.l array
layout(std430, set = 0, binding = 5) buffer readonly LightVisibility {
    uint data[];
} visible_lights;

layout(set = 0, binding = 6) uniform samplerCube irradiance_map;
layout(set = 0, binding = 7) uniform samplerCube specular_map;
layout(set = 0, binding = 8) uniform sampler2D brdf_lut;
layout(set = 0, binding = 9) uniform sampler2DArrayShadow shadow_maps[ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS];

layout(set = 0, binding = 10) uniform sampler2D textures[];

layout(push_constant) uniform PC {
    // Vertex shader
    uint transform_idx;
    // Fragment shader
    uvec2 num_tiles;
    uint albedo_idx;
    uint normal_idx;
    uint metal_rough_idx;
    uint occlusion_idx;
} pc;

// Attenuation model taken from Epic Games
float calculate_attenuation(PointLight light) {
    float dist = length(world_pos - light.pos_radius.xyz);
//    return light.color_intensity.a / (dist * dist);
    float radius = light.pos_radius.w;
    float num = saturate(1 - pow(dist / radius, 4));
    return light.color_intensity.a * num * num / (dist * dist + 1);
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

// Computes the specular falloff of a point at a certain distance to a light with a given radius.
// Note that this value isn't completely physically accurate, it's only there to mitigate the issue
// where part of a specular highlight is culled by the light radius.
// This function returns a value that is then multiplied with the specular strength to get the final result
// (so we return 1.0 when there is no falloff, and 0.0 when the light is completely dark).
// An explanation of the math used below:
//
// We define d as the falloff point, the ratio of the light's radius where the falloff will start.
// The input to the system is x = distance / radius.
// Let e(x) be any easing function. We'll start by inverting this easing function to get a value from 1 to 0 instead,
// so we use 1 - e(x).
// The falloff function f(x) is then 1.0 when x <= d, and 1 - e(x) when x > d.
// We will now squish e(x) along the x-axis to fit the [d, 1] range and name this function e'(x) Additionally we'll move it
// to the right so that e'(d) = 1.
// This gives e'(x) = 1 - e(ax - d). a is the scaling factor, and logically follows that a = 1 / (1 - d).
// Thus e'(x) = 1 - e(x / (1 - d) - d).
// Finally we need to plug this into a singular function by combining this with the values of f(x) for x <= d.
// Since we know the values of e'(x) are in the range [1, 0], we can simply say that
// f(x) = min(1, e'(x)).
float specular_falloff(float distance, float radius) {
    const float d = 0.7; // falloff point
    float x = distance / radius;
    float e = 1.0 - ease_out_quadratic(x / (1.0 - d) - d);
    return min(1.0, e);
}

// Apply metallic/roughness PBR shading
vec3 apply_point_light(PointLight light, vec3 normal, vec3 albedo, float metallic, float roughness) {
    vec3 light_dir = normalize(light.pos_radius.xyz - world_pos);
    vec3 view_dir = normalize(camera.position.xyz - world_pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);

    vec3 radiance = light.color_intensity.rgb * calculate_attenuation(light);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnel_schlick(max(dot(halfway_dir, view_dir), 0.0), F0);

    // Calculate Cook-Torrance BRDF.
    float NDF = distribution_ggx(normal, halfway_dir, roughness);
    float G = geometry_smith(normal, view_dir, light_dir, roughness);

    vec3 num = NDF * G * F;
    float denom = 4.0 * max(dot(normal, view_dir), 0.0) * max(dot(normal, light_dir), 0.0);
    vec3 specular = num / max(denom, 0.001); // Make sure to not divide by zero

    // Note that we are guaranteed that distance <= radius due to the light culling algorithm.
    specular = specular * specular_falloff(length(light.pos_radius.xyz - world_pos), light.pos_radius.w);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(normal, light_dir), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 apply_directional_light(DirectionalLight light, vec3 normal, vec3 albedo, float metallic, float roughness) {
    vec3 light_dir = -light.direction_shadow.xyz;
    vec3 view_dir = normalize(camera.position.xyz - world_pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);

    vec3 radiance = light.color_intensity.rgb * light.color_intensity.a;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnel_schlick(max(dot(halfway_dir, view_dir), 0.0), F0);

    // Calculate Cook-Torrance BRDF.
    float NDF = distribution_ggx(normal, halfway_dir, roughness);
    float G = geometry_smith(normal, view_dir, light_dir, roughness);

    vec3 num = NDF * G * F;
    float denom = 4.0 * max(dot(normal, view_dir), 0.0) * max(dot(normal, light_dir), 0.0);
    vec3 specular = num / max(denom, 0.001); // Make sure to not divide by zero

    // No falloff for directional lights as they are infinitely far.

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(normal, light_dir), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 calculate_indirect_light(vec3 normal, vec3 albedo, float ao, float roughness, float metallic) {
    return vec3(0);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 view_dir = normalize(camera.position.xyz - world_pos);

    vec3 F = fresnel_schlick_roughness(max(dot(normal, view_dir), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= (1.0 - metallic);
    vec3 irradiance = texture(irradiance_map, normal).rgb;
    vec3 diffuse = irradiance * albedo;

    vec3 reflect_dir = reflect(-view_dir, normal);
    const float MAX_REFLECTION_LOD = float(textureQueryLevels(specular_map));
    float lod = MAX_REFLECTION_LOD * roughness;

    vec3 indirect_specular = textureLod(specular_map, reflect_dir, lod).rgb;
    vec2 brdf_uv = vec2(max(dot(normal, view_dir), 0.0), roughness);
    brdf_uv.y = 1.0 - brdf_uv.y; // our BRDF texture is stored upside down for some reason so we invert the y coordinate
    vec2 env_brdf = texture(brdf_lut, brdf_uv).rg;
    vec3 specular = indirect_specular * (F * env_brdf.x + env_brdf.y);

    return (kD * diffuse + specular) * ao;
}

const mat4 shadow_bias = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
);

float sample_shadow(vec4 coord, vec2 offset, uint light_index, uint cascade) {
    float shadow = 1.0;
    const float biases[4] = {0.01, 0.005, 0.002, 0.001};

    if (coord.z > -1.0 && coord.z < 1.0) {
//        float dist = texture(shadow_maps[light_index], vec3(coord.xy + offset, cascade)).r;
        float dist = 0.0;
        if (coord.w > 0 && dist < coord.z - biases[cascade]) {
            shadow = 0.5f; // shadowed
        }
    }
    return shadow;
}

float filter_shadow_pcf(vec4 coord, uint light_index, uint cascade) {
    ivec2 dim = textureSize(shadow_maps[light_index], 0).xy;
    float scale = 0.75;
    float dx = scale * 1.0 / float(dim.x);
    float dy = scale * 1.0 / float(dim.y);

    float shadow_factor = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++) {
        for (int y = -range; y <= range; y++) {
            shadow_factor += sample_shadow(coord, vec2(dx*x, dy*y), light_index, cascade);
            count++;
        }
    }
    return shadow_factor / count;
}

float shadow_sample(vec2 base_uv, float u, float v, vec2 texel_size, uint cascade, uint light, float depth, vec2 rpdb) {
    vec2 uv = base_uv + vec2(u, v) * texel_size;
    float z = depth + dot(vec2(u, v) * texel_size, rpdb);
    vec4 s = textureGather(shadow_maps[light], vec3(uv, cascade), z);
    return (s.x + s.y + s.z + s.w) * 0.25;
}

uint used_cascade = 0;

float get_shadow_filtered(vec3 pos, vec3 mv_pos, uint light) {
    const float biases[4] = {0.01, 0.005, 0.002, 0.001};

    bool ranges[3] = {
        mv_pos.z < cascade_map_info.maps[light].splits[0],
        mv_pos.z < cascade_map_info.maps[light].splits[1],
        mv_pos.z < cascade_map_info.maps[light].splits[2],
    };

    uint cascade = 0;
    cascade += 1 * uint(ranges[0] && !ranges[1] && !ranges[2]);
    cascade += 2 * uint(ranges[1] && !ranges[2]);
    cascade += 3 * uint(ranges[2]);
//    cascade = 1;

    used_cascade = cascade;

    vec4 sc = (shadow_bias * cascade_map_info.maps[light].proj_view[cascade]) * vec4(pos, 1.0);
    sc = sc / sc.w;

    float z = sc.z;
    vec3 dx = dFdx(sc.xyz);
    vec3 dy = dFdy(sc.xyz);

    vec2 size = textureSize(shadow_maps[light], 0).xy;
    vec2 texel_size = 1.0 / size;

    vec2 rpdb = vec2(0.0);
    z -= biases[cascade];

    vec2 uv = sc.xy * size;

    vec2 base_uv;
    base_uv.x = floor(uv.x + 0.5);
    base_uv.y = floor(uv.y + 0.5);

    float s = (uv.x + 0.5 - base_uv.x);
    float t = (uv.y + 0.5 - base_uv.y);

    base_uv -= vec2(0.5);
    base_uv *= texel_size;

    return shadow_sample(base_uv, 0.0, 0.0, texel_size, cascade, light, z, rpdb);
}

void main() {
    // Determine tile index to retrieve lighting information
    const ivec2 pixel = ivec2(gl_FragCoord.xy);
    const ivec2 tile_id = pixel / ivec2(ANDROMEDA_TILE_SIZE, ANDROMEDA_TILE_SIZE);
    const uint tile = tile_id.y * pc.num_tiles.x + tile_id.x;

    vec3 albedo = texture(textures[pc.albedo_idx], UV).rgb;
    vec3 color = vec3(0.0);

    vec3 normal = normalize(TBN * (texture(textures[pc.normal_idx], UV).rgb * 2.0f - 1.0f));
    vec2 rough_metal = texture(textures[pc.metal_rough_idx], UV).gb;
    float metallic = rough_metal.y;
    float roughness = rough_metal.x;
    float ao = texture(textures[pc.occlusion_idx], UV).r;

    // Offset for this tile's information in the visible_lights array
    const uint offset = ANDROMEDA_MAX_LIGHTS_PER_TILE * tile;
    // Loop over every entry in the visible_lights array
    for (uint i = 0; (i < ANDROMEDA_MAX_LIGHTS_PER_TILE) && (visible_lights.data[offset + i] != uint(-1)); ++i) {
        const uint index = visible_lights.data[offset + i];
        PointLight light = lights.l[index];
        color += apply_point_light(light, normal, albedo, metallic, roughness);
    }


    // Process directional lights and shadowing
    for (uint i = 0; i < dir_lights.l.length(); ++i) {
        DirectionalLight light = dir_lights.l[i];
        vec3 light_color = apply_directional_light(light, normal, albedo, metallic, roughness);
        // Skip non-shadowing lights
        if (light.direction_shadow.w < 0) {
            color += light_color;
            continue;
        }
        // Find cascade index based on depth splits
        uint cascade = 0;
        uint index = uint(light.direction_shadow.w);

        for (int c = 0; c < ANDROMEDA_SHADOW_CASCADE_COUNT - 1; ++c) {
            if (viewspace_pos.z < cascade_map_info.maps[index].splits[c]) {
                cascade = c + 1;
            }
        }

//        cascade = 0;

//        vec4 shadow_coord = (shadow_bias * cascade_map_info.maps[index].proj_view[cascade]) * vec4(world_pos, 1.0);
//        float shadow = filter_shadow_pcf(shadow_coord / shadow_coord.w, index, cascade);

        float shadow = get_shadow_filtered(world_pos, viewspace_pos, index);
        light_color *= shadow;
        color += light_color;

//       color = vec3(texture(shadow_maps[index], vec3((shadow_coord / shadow_coord.w).xy, cascade)).r);

        // color cascades
        if (true) {
            switch(used_cascade) {
                case 0 :
                color.rgb *= vec3(1.0f, 0.25f, 0.25f);
                break;
                case 1 :
                color.rgb *= vec3(0.25f, 1.0f, 0.25f);
                break;
                case 2 :
                color.rgb *= vec3(0.25f, 0.25f, 1.0f);
                break;
                case 3 :
                color.rgb *= vec3(1.0f, 1.0f, 0.25f);
                break;
            }
        }
    }

    // Add indirect light from the environment to the final color
    color += calculate_indirect_light(normal, albedo, ao, roughness, metallic);

    FragColor = vec4(color, 1.0);
}