#version 460

#extension GL_GOOGLE_include_directive : enable

#include "include/glsl/inputs.glsl"

layout(location = 0) in vec3 iPos;
// other attributes ignored

layout(set = 0, binding = 1) buffer readonly Transforms {
    mat4 data[];
} transforms;

layout(push_constant) uniform PC {
    uint transform_idx;
} pc;

void main() {
    gl_Position = camera.pv * transforms.data[pc.transform_idx] * vec4(iPos, 1.0);
}
