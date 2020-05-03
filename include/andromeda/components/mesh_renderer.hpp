#ifndef ANDROMEDA_MESH_RENDERER_COMPONENT_HPP_
#define ANDROMEDA_MESH_RENDERER_COMPONENT_HPP_

#include <andromeda/assets/material.hpp>
#include <andromeda/util/handle.hpp>

namespace andromeda {
namespace components {

struct [[component]] MeshRenderer {
	Handle<Material> material;
};

}
}

#endif