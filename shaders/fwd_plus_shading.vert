#version 450

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec3 iNormal;
layout(location = 2) in vec3 iTangent;
layout(location = 3) in vec2 iTexCoords;

layout(location = 0) out vec2 UV;
layout(location = 1) out vec3 world_pos;
layout(location = 2) out mat3 TBN;

layout(set = 0, binding = 0) uniform Camera {
	mat4 view;
	mat4 projection;
	mat4 projection_view;
	vec3 cam_pos;
} camera;

layout(std430, set = 0, binding = 1) buffer readonly TransformData {
    mat4 models[];
} transforms;

layout(push_constant) uniform PC {
    uint transform_idx;
    uint albedo_idx;
    uint normal_idx;
    uint metallic_idx;
    uint roughness_idx;
    uint ambient_occlusion_idx;
    uint tile_count_x;
} pc;

void main() {
    mat4 model = transforms.models[pc.transform_idx];
    mat3 normal = transpose(inverse(mat3(model)));
    vec3 T = normalize(vec3(model * vec4(iTangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(iNormal, 0.0)));
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);
    UV = iTexCoords;
    world_pos = (model * vec4(iPos, 1.0)).xyz;
    gl_Position = camera.projection_view * model * vec4(iPos, 1.0);
}