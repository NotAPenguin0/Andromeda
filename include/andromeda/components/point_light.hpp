#pragma once

#include <glm/vec3.hpp>

namespace andromeda {

struct [[component]] PointLight {
    [[editor::tooltip("Radius of the light in worldspace units")]]
    [[editor::drag_speed(0.1f)]]
    [[editor::min(0.0f)]]
    float radius = 1.0f;

    [[editor::tooltip("Color of the light.")]]
    [[editor::drag_speed(0.05f)]]
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

    [[editor::tooltip("Intensity multiplier for the light. Increasing this does not increase the radius of the light.")]]
    [[editor::drag_speed(0.1f)]]
    float intensity = 1.0f;
};

}