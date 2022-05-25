#version 450

#include "include/glsl/common.glsl"
#include "include/glsl/inputs.glsl"

// Shader adapted from https://www.shadertoy.com/view/stSGRy

// TODO: Possibly move computations into look-up textures (see Hillaire). We'll only do this after the main atmosphere rendering is working.
// TODO: Clean up code

#define AS_MAIN_SAMPLES 16
#define AS_TRANSMITTANCE_SAMPLES 8


layout(location = 0) in vec2 UV;

layout(push_constant) uniform PC {
    vec4 sun_dir; // can allow for multiple suns later, this is temporary while i get things set up.
} pc;

layout(location = 0) out vec4 Color;

struct Atmosphere {
    float planet_radius;
    float atmosphere_radius;

    vec3 rayleigh_coeff;
    float rayleigh_scatter_height;

    vec3 mie_coeff;
    float mie_albedo;
    float mie_scatter_height;
    float mie_g;

    vec3 ozone_coeff;

    mat2x3 scatter_coeff;
    mat3x3 extinction_coeff;

    float sun_illuminance;
};

Atmosphere make_earth_atmosphere() {
    Atmosphere atm;
    atm.planet_radius = 6371e3; // radius of the earth
    atm.atmosphere_radius = 6471e3; // 100km high atmosphere

    atm.rayleigh_coeff = vec3(5.8e-6, 13.3e-6, 33.31e-6);
    atm.rayleigh_scatter_height = 8e3;

    atm.mie_coeff = vec3(21e-6);
    atm.mie_albedo = 0.9;
    atm.mie_scatter_height = 1.2e3;
    atm.mie_g = 0.8; // g parameter for phase function

    atm.ozone_coeff = vec3(7.7295962e-7, 6.67717648e-7, 7.04931588e-8); // ozone absorption

    atm.scatter_coeff = mat2x3(atm.rayleigh_coeff, atm.mie_coeff);

    atm.extinction_coeff[0] = atm.rayleigh_coeff; // Rayleigh
    atm.extinction_coeff[1] = atm.mie_coeff / atm.mie_albedo; // Mie
    atm.extinction_coeff[2] = atm.ozone_coeff; // Ozone

    atm.sun_illuminance = 22.0; // sun light intensity

    return atm;
}

// get world position and convert to position relative to earth center
vec3 relative_to_planet(Atmosphere atm, vec3 p) {
    return vec3(0, p.y + atm.planet_radius, 0);
}

// Add a small offset to the atmosphere ray to avoid self-intersecting with planet.
// TODO: tweak this value.
vec3 add_ray_offset(vec3 p) {
    return p + vec3(0, 1e1, 0);
}

// todo: invert matrix on cpu instead
vec3 camera_ray_direction(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec4 target = camera.inv_projection * vec4(uv.x, uv.y, 1, 1);
    return normalize(inverse(camera.view_rotation) * vec4(normalize(target.xyz), 0)).xyz;
}

vec2 ray_sphere_intersection(vec3 origin, vec3 direction, float radius) {
    float b = dot(origin, direction);
    float c = dot(origin, origin) - radius * radius;
    float d = b * b - c;

    if (d < 0.0)
        return vec2(-1.0);

    d = sqrt(d);

    return vec2(-b - d, -b + d);
}

float rayleigh_phase(float cos_theta) {
    return (3.0 / (16.0 * PI)) * (1.0 + cos_theta * cos_theta);
}

// uses shlick approximation. note that for large g values this becomes quite inaccurate.
// (TODO: Switch to full henyey-greenstein when g > X ?)
float mie_phase(float cos_theta, float g) {
    const float k = 1.55 * g - 0.55 * (g * g * g);
    const float num = 1 - k * k;
    const float d = 1 + k * cos_theta;
    const float denom = 4 * PI * d * d;
    return num / denom;
}

vec3 get_densities(Atmosphere atm, float altitude) {
    float rayleigh = exp(-altitude / atm.rayleigh_scatter_height);
    float mie = exp(-altitude / atm.mie_scatter_height);
    float ozone = exp(-max(0.0, (35e3 - altitude) - atm.atmosphere_radius) / 5e3) * exp(-max(0.0, (altitude - 35e3) - atm.atmosphere_radius) / 15e3);

    return vec3(rayleigh, mie, ozone);
}

vec3 compute_light_transmittance(Atmosphere atm, vec3 position, vec3 light_direction) {
    // get intersection with atmosphere
    vec2 atm_intersect = ray_sphere_intersection(position, light_direction, atm.atmosphere_radius);
    const float dt = atm_intersect.y / float(AS_TRANSMITTANCE_SAMPLES);
    const vec3 dr = light_direction * dt;

    // initial ray position
    vec3 ray_position = position + dr * 0.5;
    // initialize accumulator
    vec3 total_transmittance = vec3(1.0);

    for (int i = 0; i < AS_TRANSMITTANCE_SAMPLES; ++i) {
        // TODO: same code as in get_sky_color, should move this to a function
        const float altitude = length(ray_position) - atm.planet_radius;
        const vec3 density = get_densities(atm, altitude);
        const vec3 air_mass = density * dt;
        const vec3 optical_depth = atm.extinction_coeff * air_mass;
        const vec3 transmittance = exp(-optical_depth);
        total_transmittance += transmittance;

        ray_position += dr;
    }
    return total_transmittance;
}

vec3 get_sky_color(Atmosphere atm, vec3 ray_origin, vec3 ray_direction, vec3 light_direction) {
    // get intersection points with planet and atmosphere to determine step size.
    vec2 atm_intersect = ray_sphere_intersection(ray_origin, ray_direction, atm.atmosphere_radius);
    vec2 planet_intersect = ray_sphere_intersection(ray_origin, ray_direction, atm.planet_radius);

    bool planet_intersected = planet_intersect.y >= 0.0;
    bool atmosphere_intersected = atm_intersect.y >= 0.0;
    vec2 sd = vec2(
        (planet_intersected && planet_intersect.x < 0.0) ? planet_intersect.y : max(atm_intersect.x, 0.0),
        (planet_intersected && planet_intersect.x > 0.0) ? planet_intersect.x : atm_intersect.y);

    // compute step size
    float dt = length(sd.y - sd.x) / float(AS_MAIN_SAMPLES);
    // ray increment
    vec3 dr = ray_direction * dt;

    // start position for ray
    vec3 ray_position = ray_direction * sd.x + (dr * 0.5 + ray_origin);

    // initialize variables for accumulating scattering and transmittance
    vec3 total_scattering = vec3(0.0);
    vec3 total_transmittance = vec3(1.0);

    // compute rayleigh and mie phase functions
    float cos_theta = dot(ray_direction, light_direction);
    vec2 phases = vec2(rayleigh_phase(cos_theta), mie_phase(cos_theta, atm.mie_g));

    // now we will march along the path of the view ray to compute the sky color for this pixel.
    for (int i = 0; i < AS_MAIN_SAMPLES; ++i) {
        // compute altitude of this sample point, for sampling density functions.
        const float altitude = length(ray_position) - atm.planet_radius;
        // get density and air mass
        const vec3 density = get_densities(atm, altitude);
        const vec3 air_mass = density * dt;
        // with that we can compute the optical depth
        const vec3 optical_depth = atm.extinction_coeff * air_mass;
        // ... which is in turn the transmittance
        const vec3 transmittance = exp(-optical_depth);
        // sample total transmittance along light ray (!= current ray)
        const vec3 light_transmittance = compute_light_transmittance(atm, ray_position, light_direction);
        // single scattering
        const vec3 scattering = atm.scatter_coeff * (phases.xy * air_mass.xy) * light_transmittance;
        // we can use this to solve scattering integral (method from frostbite: https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf)
        const vec3 scattering_integral = (scattering - scattering * transmittance) / max(vec3(1e-8), optical_depth);

        // accumulate variables and move ray
        total_scattering += scattering_integral * total_transmittance;
        total_transmittance += transmittance;

        ray_position += dr;
    }

    return atm.sun_illuminance * total_scattering;
}

void main() {
    Atmosphere atm = make_earth_atmosphere();

    vec3 view_pos = relative_to_planet(atm, camera.position.xyz);
//    vec3 ray_origin = add_ray_offset(view_pos);
    float offset = 1e1;
    vec3 ray_origin = vec3(0.0, 6371e3 + offset, 0.0);
    vec3 ray_direction = camera_ray_direction(UV);
    vec3 light_direction = pc.sun_dir.xyz;

    vec3 color = get_sky_color(atm, ray_origin, ray_direction, light_direction);

    Color = vec4(color, 1.0);
}