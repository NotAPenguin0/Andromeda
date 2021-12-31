#define PI 3.14159265358979

float linearize_depth(float d, float near, float far) {
    return near * far / (far + d * (near - far));
}

vec2 near_far_decompose(mat4 perspective) {
    float near = (1.0 + perspective[3][2]) / perspective[2][2];
    float far = - (1.0 - perspective[3][2]) / perspective[2][2];
    return vec2(near, far);
}

// All components are in the range [0...1], including hue.
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// All components in range [0...1]
vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

float rgb2lum(vec3 rgb) {
    return 0.2125 * rgb.r + 0.7154 * rgb.g + 0.0721 * rgb.b;
}

// Conversions below are taken from https://github.com/tobspr/GLSL-Color-Spaces/blob/master/ColorSpaces.inc.glsl

const float SRGB_GAMMA = 1.0 / 2.2;
const float SRGB_INVERSE_GAMMA = 2.2;
const float SRGB_ALPHA = 0.055;


// Used to convert from linear RGB to XYZ space
const mat3 RGB_2_XYZ = (mat3(
    0.4124564, 0.2126729, 0.0193339,
    0.3575761, 0.7151522, 0.1191920,
    0.1804375, 0.0721750, 0.9503041
));

// Used to convert from XYZ to linear RGB space
const mat3 XYZ_2_RGB = (mat3(
    3.2404542,-0.9692660, 0.0556434,
    -1.5371385, 1.8760108,-0.2040259,
    -0.4985314, 0.0415560, 1.0572252
));

// Converts a linear rgb color to a srgb color (approximated, but fast)
vec3 rgb2srgb_approx(vec3 rgb) {
    return pow(rgb, vec3(SRGB_GAMMA));
}

// Converts a srgb color to a rgb color (approximated, but fast)
vec3 srgb2rgb_approx(vec3 srgb) {
    return pow(srgb, vec3(SRGB_INVERSE_GAMMA));
}

// Converts a single linear channel to srgb
float linear2srgb(float channel) {
    if(channel <= 0.0031308)
    return 12.92 * channel;
    else
    return (1.0 + SRGB_ALPHA) * pow(channel, 1.0/2.4) - SRGB_ALPHA;
}

// Converts a single srgb channel to rgb
float srgb2linear(float channel) {
    if (channel <= 0.04045)
    return channel / 12.92;
    else
    return pow((channel + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);
}

// Converts a linear rgb color to a srgb color (exact, not approximated)
vec3 rgb2srgb(vec3 rgb) {
    return vec3(
        linear2srgb(rgb.r),
        linear2srgb(rgb.g),
        linear2srgb(rgb.b)
    );
}

// Converts a srgb color to a linear rgb color (exact, not approximated)
vec3 srgb2rgb(vec3 srgb) {
    return vec3(
        srgb2linear(srgb.r),
        srgb2linear(srgb.g),
        srgb2linear(srgb.b)
    );
}

// Converts a color from linear RGB to XYZ space
vec3 rgb2xyz(vec3 rgb) {
    return RGB_2_XYZ * rgb;
}

// Converts a color from XYZ to linear RGB space
vec3 xyz2rgb(vec3 xyz) {
    return XYZ_2_RGB * xyz;
}

// Converts a color from XYZ to xyY space (Y is luminosity)
vec3 xyz2xyY(vec3 xyz) {
    float Y = xyz.y;
    float x = xyz.x / (xyz.x + xyz.y + xyz.z);
    float y = xyz.y / (xyz.x + xyz.y + xyz.z);
    return vec3(x, y, Y);
}

// Converts a color from xyY space to XYZ space
vec3 xyY2xyz(vec3 xyY) {
    float Y = xyY.z;
    float x = Y * xyY.x / xyY.y;
    float z = Y * (1.0 - xyY.x - xyY.y) / xyY.y;
    return vec3(x, Y, z);
}

// Converts a color from linear RGB to xyY space
vec3 rgb2xyY(vec3 rgb) {
    vec3 xyz = rgb2xyz(rgb);
    return xyz2xyY(xyz);
}

// Converts a color from xyY space to linear RGB
vec3 xyY2rgb(vec3 xyY) {
    vec3 xyz = xyY2xyz(xyY);
    return xyz2rgb(xyz);
}

// To srgb
vec3 xyz2srgb(vec3 xyz)  { return rgb2srgb(xyz2rgb(xyz)); }
vec3 xyY2srgb(vec3 xyY)  { return rgb2srgb(xyY2rgb(xyY)); }

vec3 srgb2xyz(vec3 srgb) { return rgb2xyz(srgb2rgb(srgb)); }

vec3 srgb2xyY(vec3 srgb) { return rgb2xyY(srgb2rgb(srgb)); }


// Claps a value to [0...1]
float saturate(float x) {
    return max(0.0, min(x, 1.0));
}

// Quadratically eases a value between 0 and 1.
// This implements the easing function at https://easings.net/#easeOutQuad
float ease_out_quadratic(float x) {
    return 1.0 - (x - 1.0) * (x - 1.0);
}

vec3 uncharted2_tonemap_partial(vec3 x) {
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_tonemap_filmic(vec3 v) {
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces_tonemap(vec3 x) {
    x *= 0.6; //
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Version of ACES tonemap that only operates on a luminance value.
float aces_tonemap(float x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
