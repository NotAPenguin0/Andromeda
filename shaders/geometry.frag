#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) out vec4 gNormal;
layout (location = 1) out vec4 gAlbedoSpec;

layout(location = 0) in vec2 UV;
layout(location = 1) in mat3 TBN;

layout(set = 0, binding = 2) uniform sampler2D textures[];

layout(push_constant) uniform PC {
    uint transform_idx;
    uint diffuse_idx;
    uint normal_idx;
} pc;

// Deferred example adapted from learnopengl.com

void main() {    
    // Store the per-fragment normals into the gbuffer
    vec3 norm = texture(textures[pc.normal_idx], UV).rgb * 2.0 - 1.0;
    gNormal.xyz = normalize(TBN * norm) * 0.5 + 0.5;
    gNormal.w = 1.0;
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(textures[pc.diffuse_idx], UV).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
//    gAlbedoSpec.a = texture(textures[indices.specular_index], UB).r;
    gAlbedoSpec.a = 1.0f;
}