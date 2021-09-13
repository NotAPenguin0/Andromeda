#pragma once 

#include <glm/vec3.hpp>

namespace andromeda {

struct [[component]] Transform {
	[[editor::tooltip("Position relative to parent.")]]
	[[editor::drag_speed(0.5f)]]
	glm::vec3 position{};

	[[editor::tooltip("Rotation relative to parent, in degrees")]]
	[[editor::drag_speed(0.5f)]]
	glm::vec3 rotation{};

	[[editor::tooltip("Scale relative to parent.")]]
	[[editor::drag_speed(0.1f)]]
	glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
};

}