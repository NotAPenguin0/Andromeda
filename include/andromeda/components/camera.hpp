#pragma once

#include <glm/vec3.hpp>

namespace andromeda {

/**
 * @struct Camera 
 * @brief Camera component.
*/
struct [[component]] Camera {
	[[editor::tooltip("Camera field of view in degrees.")]]
	float fov = 90.0f;

	[[editor::tooltip("Distance to the near clipping plane.")]]
	float near = 0.01f;

	[[editor::tooltip("Distance to the far clipping plane.")]]
	float far = 100.0f;
};

}