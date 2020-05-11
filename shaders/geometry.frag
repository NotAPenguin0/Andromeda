#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) out vec4 gNormal;
layout (location = 1) out vec4 gAlbedoAO;
layout (location = 2) out vec2 gMetallicRoughness;

layout(location = 0) in vec2 UV;
layout(location = 1) in mat3 TBN;

layout(set = 0, binding = 2) uniform sampler2D textures[];

layout(push_constant) uniform PC {
    uint transform_idx;
    uint albedo_idx;
    uint normal_idx;
    uint metallic_idx;
    uint roughness_idx;
    uint ambient_occlusion_idx;
} pc;

// Deferred example adapted from learnopengl.com

void main() {    
    // Store the per-fragment normals into the gbuffer
    vec3 norm = texture(textures[pc.normal_idx], UV).rgb * 2.0 - 1.0;
    gNormal.xyz = normalize(TBN * norm) * 0.5 + 0.5;
    gNormal.w = 1.0;
    // and the diffuse per-fragment color
    gAlbedoAO.rgb = texture(textures[pc.albedo_idx], UV).rgb;
    gAlbedoAO.a = texture(textures[pc.ambient_occlusion_idx], UV).r;

    // Only the red and green channels for gMetallicRoughness are used. Other channels are reserved for later use (or might be taken away completely).
    gMetallicRoughness.r = texture(textures[pc.metallic_idx], UV).r;
    gMetallicRoughness.g = texture(textures[pc.roughness_idx], UV).r;
}