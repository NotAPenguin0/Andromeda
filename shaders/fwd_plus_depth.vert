#version 450

layout(location = 0) in vec3 iPos;

layout(set = 0, binding = 0) uniform CameraData {
	mat4 view;
	mat4 projection;
    mat4 projection_view;
} camera;

layout(std430, set = 0, binding = 1) buffer readonly TransformData {
    mat4 models[];
} transforms;

layout(push_constant) uniform PC {
    uint transform_idx;
} pc;

void main() {
    mat4 model = transforms.models[pc.transform_idx];
    gl_Position = camera.projection_view * model * vec4(iPos, 1.0);
}