#version 450

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform PC {
    vec4 color; // a component ignored
} pc;

void main() {
    FragColor = vec4(pc.color.rgb, 1.0);
}