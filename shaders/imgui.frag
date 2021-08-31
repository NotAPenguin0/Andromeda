#version 450 core
layout(location = 0) out vec4 fColor;

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(push_constant) uniform uPushConstant {
    vec2 uScale;
    vec2 uTranslate;
    int uDepth;
} pc;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

void main()
{
    vec4 color_corrected = vec4(pow(In.Color.rgb, vec3(2.2f)), In.Color.a);
    if (pc.uDepth == 1)
        fColor = color_corrected * texture(sTexture, In.UV.st).rrra;
    else
        fColor = color_corrected * texture(sTexture, In.UV.st);
}
