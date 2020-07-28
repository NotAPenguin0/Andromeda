#version 450

layout(location = 0) in vec2 UV;

layout(set = 0, binding = 0) uniform sampler2DMS input_hdr;

layout(location = 0) out vec4 FragColor;


vec3 uncharted2_tonemap_partial(vec3 x) {
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_filmic(vec3 v) {
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

void main() {
    const int NUM_SAMPLES = 8;
    ivec2 tex_size = ivec2(textureSize(input_hdr));
    ivec2 texels = ivec2(UV * tex_size);

    vec3 color = vec3(0);

    for (int sample_i = 0; sample_i < NUM_SAMPLES; ++sample_i) {
	    vec4 in_hdr = texelFetch(input_hdr, texels, sample_i);
        color += uncharted2_filmic(in_hdr.rgb);
    }
    FragColor = vec4(color / float(NUM_SAMPLES), 1.0);
}