#pragma once

#include <andromeda/util/handle.hpp>
#include <andromeda/graphics/forward.hpp>

namespace andromeda {

struct [[component]] MeshRenderer {
	Handle<gfx::Mesh> mesh{};
};

}