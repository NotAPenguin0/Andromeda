// This file is not compatible with C++

layout(set = 0, binding = 0) uniform Camera {
    mat4 projection;
    mat4 view;
    mat4 pv;
    vec4 position; // last value ignored, only there for alignment purposes
} camera;