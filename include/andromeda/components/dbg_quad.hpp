#ifndef ANDROMEDA_DEBUG_QUAD_COMPONENT_HPP_
#define ANDROMEDA_DEBUG_QUAD_COMPONENT_HPP_

#include <andromeda/util/handle.hpp>

namespace andromeda {
class Texture;
}

namespace andromeda::components {

struct [[component]] DbgQuad {
	Handle<Texture> texture;
};

}

#endif