#version 450

layout(location = 0) in vec3 iPos;

layout(location = 0) out vec3 UVW;

layout(push_constant) uniform PC {
    mat4 projection;
    mat4 view_no_position;
} pc;

void main() {
    UVW = iPos;
    vec4 pos = pc.projection * pc.view_no_position * vec4(iPos, 1.0);
    gl_Position = pos.xyww;
}