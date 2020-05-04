#ifndef ANDROMEDA_STATIC_MESH_COMPONENT_HPP_
#define ANDROMEDA_STATIC_MESH_COMPONENT_HPP_

#include <andromeda/util/handle.hpp>

namespace andromeda {
class Mesh;
}

namespace andromeda::components {

struct [[component]] StaticMesh {
	Handle<Mesh> mesh;
};

}

#endif