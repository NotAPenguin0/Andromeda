#pragma once

#include <andromeda/graphics/forward.hpp>
#include <andromeda/util/handle.hpp>

/**
 * @brief Environment component. Stores the gfx::Environment asset to use when rendering
 */
namespace andromeda {

struct [[component]] Environment {
    [[editor::tooltip("Environment asset to use.")]]
    Handle<gfx::Environment> environment;
};

}