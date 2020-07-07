#version 450

layout(location = 0) in vec3 UV;

layout(set = 0, binding = 1) uniform samplerCube skybox;

layout(location = 0) out vec4 FragColor;

void main() {
    vec3 color = texture(skybox, UV).rgb;
	FragColor = vec4(color, 1.0);
	gl_FragDepth = 1.0;
}