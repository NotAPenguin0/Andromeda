#version 460

#extension GL_GOOGLE_include_directive : enable

#include "include/glsl/inputs.glsl"

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec3 iTangent;
layout(location = 3) in vec2 iUV;

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

layout(location = 0) out vec2 UV;
layout(location = 1) out vec3 world_pos;
layout(location = 2) out mat3 TBN;

// These matrices are indexed through push constants
layout(set = 0, binding = 1) buffer readonly TransformMatrices {
    mat4 data[];
} transforms;

void main() {
    // Calculate TBN matrix for normal mapping.
    mat4 model = transforms.data[pc.transform_idx];
    mat3 normal = transpose(inverse(mat3(model)));
    vec3 T = normalize(vec3(model * vec4(iTangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(iNormal, 0.0)));
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    // Output data to fragment shader
    TBN = mat3(T, B, N);
    UV = iUV;
    world_pos = (model * vec4(iPos, 1.0)).xyz;
    gl_Position = camera.pv * model * vec4(iPos, 1.0);
}