#ifndef ANDROMEDA_MATERIAL_HPP_
#define ANDROMEDA_MATERIAL_HPP_

#include <andromeda/assets/texture.hpp>
#include <andromeda/util/handle.hpp>

namespace andromeda {

struct Material {
	Handle<Texture> diffuse;
	Handle<Texture> normal;
};


}

#endif