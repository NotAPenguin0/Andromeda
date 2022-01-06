#version 450

layout(location = 0) in vec3 iPos;

layout(set = 0, binding = 0) uniform Camera {
    mat4 pv;
} camera;

layout(push_constant) uniform PC {
    vec4 color; // a component ignored
} pc;

void main() {
    gl_Position = camera.pv * vec4(iPos, 1.0);
}