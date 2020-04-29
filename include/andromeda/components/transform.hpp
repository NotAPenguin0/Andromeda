#ifndef ANDROMEDA_TRANSFORM_COMPONENT_HPP_
#define ANDROMEDA_TRANSFORM_COMPONENT_HPP_

#include <glm/vec3.hpp>

namespace andromeda::components {

struct [[component]] Transform {
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};

}

#endif