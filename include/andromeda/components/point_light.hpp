#ifndef ANDROMEDA_POINT_LIGHT_COMPONENT_HPP_
#define ANDROMEDA_POINT_LIGHT_COMPONENT_HPP_

#include <glm/vec3.hpp>

namespace andromeda {
namespace components {

struct [[component]] PointLight {
	// Radius of the light in worldspace units
	float radius = 1.0f;
	// Color of the light
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	// Intensity modifier. Note that a higher intensity does not mean a larger radius.
	float intensity = 1.0f;
};

}

}

#endif