#version 450

layout(location = 0) in vec3 UVW;

layout(set = 0, binding = 0) uniform samplerCube skybox;

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform PC {
    mat4 projection;
    mat4 view_no_position;
} pc;

void main() {
    vec4 color = texture(skybox, UVW).rgba;
    FragColor = color;
}