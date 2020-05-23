#version 450

layout (location = 0) in vec3 iPos;

layout(location = 0) flat out uint light_index;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 projection_view;
    mat4 inverse_projection;
    mat4 inverse_view;
    vec3 position;
} camera;

struct PointLight {
    // First 3 values have the position, last has the radius of the light
    vec4 transform;
    // first 3 values are the color, last value has the intensity of the light
    vec4 color;
};

layout(set = 0, binding = 1) buffer readonly PointLights {
   PointLight lights[];
} lights;

layout(push_constant) uniform PC {
    uvec2 screen_size;
} pc;

void main() {
    light_index = gl_InstanceIndex;
    PointLight light = lights.lights[gl_InstanceIndex];
    // Since we don't need rotations, we can get away with using simple math instead of transformation matrices for our lights
    vec4 light_world = vec4(light.transform.w * iPos + light.transform.xyz, 1.0);
    vec4 light_clip = camera.projection_view * light_world;
    gl_Position = light_clip;
}