#version 450
layout(location = 0) out vec4 fColor;

#extension GL_EXT_scalar_block_layout : enable

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(scalar, push_constant) uniform uPushConstant {
    vec2 uScale;
    vec2 uTranslate;
    uint uDepth;
//    int uUserTex; // color correct if false
} pc;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

void main()
{
    vec4 color = In.Color;
//    if (pc.uUserTex == 0) {
        color = vec4(pow(color.rgb, vec3(2.2f)), color.a);
//    }
    if (pc.uDepth == 1)
        fColor = color * vec4(texture(sTexture, In.UV.st).rrr, 1.0);
    else
        fColor = color * texture(sTexture, In.UV.st);
}
