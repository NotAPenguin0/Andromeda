#version 460

#extension GL_GOOGLE_include_directive : enable

#include "include/glsl/inputs.glsl"

layout(location = 0) in vec3 iPos;
// other attributes ignored

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {
    gl_Position = camera.pv * pc.model * vec4(iPos, 1.0);
}
