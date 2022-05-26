#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/glsl/common.glsl"

layout(location = 0) in vec2 UV;

layout(set = 0, binding = 0) uniform sampler2DMS input_hdr;
layout(set = 0, binding = 1) buffer readonly AverageLuminance {
    float value;
} average_luminance;

layout(push_constant) uniform PC {
    uint samples;
} pc;

layout(location = 0) out vec4 FragColor;

void main() {
    const uint NUM_SAMPLES = pc.samples;
    ivec2 tex_size = ivec2(textureSize(input_hdr));
    ivec2 texels = ivec2(UV * tex_size);

    vec3 color = vec3(0.0);

    // Automatic exposure, we'll scale all our luminance values with a factor H
    float avg_lum = average_luminance.value;
    const float K = 12.5;
    const float S = 100.0;
    const float q = 0.65;
    const float ev100 = log2(avg_lum * (S / K));
    const float l = (78.0 / (q * S)) * pow(2, ev100);
    const float H = 1.0 / l;

    for (int sample_i = 0; sample_i < NUM_SAMPLES; ++sample_i) {
        vec3 in_hdr = texelFetch(input_hdr, texels, sample_i).rgb;
        vec3 xyY = srgb2xyY(in_hdr.rgb);
        float lum = xyY.b * H;
        lum = aces_tonemap(lum);
        xyY.z = lum;
        color += xyY2srgb(xyY);
    }
    FragColor = vec4(color / float(NUM_SAMPLES), 1.0);
}