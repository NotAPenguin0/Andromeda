#pragma once

#include <andromeda/util/handle.hpp>
#include <andromeda/graphics/forward.hpp>

namespace andromeda::gfx {

struct Material {
    // Base color of the material
    Handle<Texture> albedo;
    // Normal map
    Handle<Texture> normal;
    // Metallic/roughness map. Roughness in G component, metallic in B component
    Handle<Texture> metal_rough;
    // Ambient occlusion map
    Handle<Texture> occlusion;
};

}