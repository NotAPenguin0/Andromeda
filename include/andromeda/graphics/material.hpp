#pragma once

#include <andromeda/util/handle.hpp>
#include <andromeda/graphics/forward.hpp>

namespace andromeda::gfx {

struct Material {
	Handle<Texture> albedo;
};

}