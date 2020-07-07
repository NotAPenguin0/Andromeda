#version 450

layout(location = 0) in vec2 UV;

layout(set = 0, binding = 0) uniform sampler2D input_hdr;

layout(location = 0) out vec4 FragColor;

vec3 reinhard_extended(vec3 v, float max_white) {
    vec3 numerator = v * (1.0f + (v / vec3(max_white * max_white)));
    return numerator / (1.0f + v);
}

float luminance(vec3 v)
{
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 reinhard_jodie(vec3 v)
{
    float l = luminance(v);
    vec3 tv = v / (1.0f + v);
    return mix(v / (1.0f + l), tv, tv);
}

vec3 uncharted2_tonemap_partial(vec3 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_filmic(vec3 v)
{
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

void main() {
	vec4 in_hdr = texture(input_hdr, UV);
    vec3 out_ldr = uncharted2_filmic(in_hdr.rgb);
    FragColor = vec4(out_ldr, 1.0);
}