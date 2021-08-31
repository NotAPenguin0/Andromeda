#pragma once 

#include <glm/vec3.hpp>

namespace andromeda {

struct [[component]] Transform {
	glm::vec3 position{};
	glm::vec3 rotation{};
	glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
};

}