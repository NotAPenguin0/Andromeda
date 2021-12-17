#pragma once

#include <andromeda/util/handle.hpp>
#include <andromeda/graphics/forward.hpp>

namespace andromeda::gfx {

struct Material {
    // Base color of the material
	Handle<Texture> albedo;
    // Normal map
    Handle<Texture> normal;
    // Metallic map
    Handle<Texture> metallic;
    // Roughness map
    Handle<Texture> roughness;
    // Ambient occlusion map
    Handle<Texture> occlusion;
};

}