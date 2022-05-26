#pragma once

#include <andromeda/graphics/forward.hpp>
#include <andromeda/util/handle.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/**
 * @brief Environment component. Stores the gfx::Environment asset to use when rendering
 */
namespace andromeda {

struct [[component]] Environment {
    [[editor::tooltip("Environment asset to use.")]]
    Handle<gfx::Environment> environment;

    [[editor::tooltip("Radius of the planet.")]]
    [[editor::drag_speed(1000.0)]]
    float planet_radius = 6371e3;

    [[editor::tooltip("Radius of the atmosphere (starting from planet center)")]]
    [[editor::drag_speed(1000.0)]]
    float atmosphere_radius = 6471e3; // todo: maybe switch this from starting at surface?

    [[editor::tooltip("Rayleigh scattering coefficients.")]]
    [[editor::drag_speed(0.0000001)]]
    [[editor::format("%.7f")]]
    glm::vec3 rayleigh = glm::vec3(5.8e-6, 13.3e-6, 33.31e-6);

    [[editor::tooltip("Rayleigh scattering height.")]]
    [[editor::drag_speed(100.0)]]
    float rayleigh_height = 8e3;

    [[editor::tooltip("Mie scattering coefficients.")]]
    [[editor::drag_speed(0.0000001)]]
    [[editor::format("%.7f")]]
    glm::vec3 mie = glm::vec3(21e-6);


    [[editor::tooltip("Mie albedo color.")]]
    [[editor::drag_speed(0.1)]]
    float mie_albedo = 0.9;

    [[editor::tooltip("Mie scattering height.")]]
    [[editor::drag_speed(100.0)]]
    float mie_height = 1.2e3;

    [[editor::tooltip("Mie asymmetry.")]]
    [[editor::max(0.99999)]]
    [[editor::drag_speed(0.005)]]
    float mie_g = 0.8; // todo: figure out why min(-0.99999) doesnt work in codegen

    [[editor::tooltip("Ozone scattering coefficients.")]]
    [[editor::drag_speed(0.000000001)]]
    [[editor::format("%.9f")]]
    glm::vec3 ozone = glm::vec3(7.7295962e-7, 6.67717648e-7, 7.04931588e-8);

    [[editor::tooltip("Intensity of the sun.")]]
    [[editor::drag_speed(0.2)]]
    float sun_intensity = 22.0;
};

}