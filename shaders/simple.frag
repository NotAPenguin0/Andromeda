#version 460

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform PushConstants {
	mat4 model;
} pc;

void main() {
	FragColor = vec4(0, 1, 0, 1);
}