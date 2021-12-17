#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/glsl/common.glsl"

layout(location = 0) in vec2 UV;

layout(set = 0, binding = 0) uniform sampler2D input_hdr;

layout(location = 0) out vec4 FragColor;

void main() {
    const int NUM_SAMPLES = 1; // TODO: multisampling?
    ivec2 tex_size = ivec2(textureSize(input_hdr, 0));
    ivec2 texels = ivec2(UV * tex_size);

    vec3 color = vec3(0.0);

    for (int sample_i = 0; sample_i < NUM_SAMPLES; ++sample_i) {
        vec4 in_hdr = texelFetch(input_hdr, texels, sample_i);
        color += uncharted2_tonemap_filmic(in_hdr.rgb);
    }
    FragColor = vec4(color / float(NUM_SAMPLES), 1.0);
}