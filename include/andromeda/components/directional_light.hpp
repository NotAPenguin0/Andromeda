#pragma once

#include <glm/vec3.hpp>

namespace andromeda {

/**
 * @brief DirectionalLight component. The direction of a light is stored in the entity's rotation in its Transform component
 */
struct [[component]] DirectionalLight {
    [[editor::tooltip("Color of the light.")]]
    [[editor::drag_speed(0.05f)]]
    glm::vec3 color;

    [[editor::tooltip("Light intensity multiplier")]]
    [[editor::drag_speed(0.1f)]]
    float intensity = 1.0f;

    [[editor::tooltip("Whether this light casts shadows.")]]
    bool cast_shadows = true;
};

}