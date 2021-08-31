#pragma once

#include <glm/vec3.hpp>

namespace andromeda {

/**
 * @struct Camera 
 * @brief Camera component.
*/
struct [[component]] Camera {
	// Field of view in degrees.
	float fov = 90.0f;

	// Distance of the near clipping plane.
	float near = 0.01f;
	// Distance of the far clipping plane.
	float far = 100.0f;
};

}