// This file is not compatible with C++

layout(set = 0, binding = 0) uniform Camera {
    mat4 projection;
    mat4 view;
    mat4 pv;
    mat4 inv_projection;
    mat4 inv_view;
    mat4 view_rotation; // view with position removed.
    vec4 position; // last value ignored, only there for alignment purposes
} camera;