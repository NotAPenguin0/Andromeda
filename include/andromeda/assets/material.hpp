#ifndef ANDROMEDA_MATERIAL_HPP_
#define ANDROMEDA_MATERIAL_HPP_

#include <andromeda/assets/texture.hpp>
#include <andromeda/util/handle.hpp>

namespace andromeda {

struct Material {
	Handle<Texture> color;
	Handle<Texture> normal;
	Handle<Texture> metallic;
	Handle<Texture> roughness;
	Handle<Texture> ambient_occlusion;
};


}

#endif