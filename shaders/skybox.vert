#version 450

layout(location = 0) in vec3 iPos;

layout(location = 0) out vec3 UV;

layout(set = 0, binding = 0) uniform Transform {
    mat4 projection;
    mat4 view;
} transform;

void main() {
    UV = iPos;
    // Remove translation part of view matrix
    mat4 view_rotation = mat4(mat3(transform.view)); 
    view_rotation[3][3] = 1;
    gl_Position = transform.projection * view_rotation * vec4(iPos, 1.0);
}