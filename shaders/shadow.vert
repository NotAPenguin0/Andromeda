#version 450

#include "include/glsl/limits.glsl"

layout(location = 0) in vec3 iPos;
// Other attributes unused

layout(set = 0, binding = 0) uniform Light {
    mat4 lightspace_pv[ANDROMEDA_SHADOW_CASCADE_COUNT];
} light;

layout(set = 0, binding = 1) buffer readonly Transforms {
    mat4 model[];
} transforms;

layout(push_constant) uniform PC {
    uint cascade_index;
    uint transform_index;
} pc;

void main() {
    gl_Position = light.lightspace_pv[pc.cascade_index] * transforms.model[pc.transform_index] * vec4(iPos, 1.0);
}