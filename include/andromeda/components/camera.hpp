#ifndef ANDROMEDA_CAMERA_COMPONENT_HPP_
#define ANDROMEDA_CAMERA_COMPONENT_HPP_

#include <glm/vec3.hpp>
#include <andromeda/util/handle.hpp>

namespace andromeda {
class EnvMap;
}

namespace andromeda::components {

struct [[component]] Camera {
	glm::vec3 front = glm::vec3(1, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);

	float fov = 0.785398f; // == radians(45°)

	Handle<EnvMap> env_map;
};

}

#endif