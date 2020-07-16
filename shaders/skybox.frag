#version 450

layout(location = 0) in vec3 UVW;

layout(set = 0, binding = 1) uniform samplerCube skybox;

layout(location = 0) out vec4 FragColor;

void main() {
    vec3 color = texture(skybox, UVW).rgb;
	FragColor = vec4(color, 1.0);
}