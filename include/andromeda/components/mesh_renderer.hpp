#pragma once

#include <andromeda/util/handle.hpp>
#include <andromeda/graphics/forward.hpp>

namespace andromeda {

struct [[component]] MeshRenderer {
    [[editor::tooltip("Mesh asset to render.")]]
    Handle <gfx::Mesh> mesh{};

    [[editor::tooltip("Material to use when rendering this mesh.")]]
    Handle <gfx::Material> material{};

    [[editor::tooltip("Whether this entity casts shadows.")]]
    bool occluder = true;
};

}