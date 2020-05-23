#ifndef ANDROMEDA_ENVIRONMENT_MAP_HPP_
#define ANDROMEDA_ENVIRONMENT_MAP_HPP_

#include <phobos/util/image_util.hpp>

namespace andromeda {

class EnvMap {
public:
	// Note: RawImage supports cubemaps.
	ph::RawImage cube_map;
	ph::ImageView cube_map_view;
	// Also a cube map
	ph::RawImage irradiance_map;
	ph::ImageView irradiance_map_view;
};

}

#endif