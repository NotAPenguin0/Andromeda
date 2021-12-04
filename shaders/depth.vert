#version 460

layout(location = 0) in vec3 iPos;
// other attributes ignored

layout(set = 0, binding = 0) uniform Camera {
    mat4 pv;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {
    gl_Position = camera.pv * pc.model * vec4(iPos, 1.0);
}
