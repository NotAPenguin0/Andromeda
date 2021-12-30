#pragma once

#include <glm/vec3.hpp>
#include <andromeda/util/handle.hpp>
#include <andromeda/graphics/forward.hpp>

namespace andromeda {
	
/**
 * @struct Camera 
 * @brief Camera component.
*/
struct [[component]] Camera {
	[[editor::tooltip("Camera field of view in degrees.")]]
	[[editor::min(0.00001f)]]
	[[editor::max(360.0f)]]
	float fov = 90.0f;

	[[editor::tooltip("Distance to the near clipping plane.")]]
	[[editor::min(0.00001f)]]
	[[editor::drag_speed(0.2)]]
	float near = 0.01f;

	[[editor::tooltip("Distance to the far clipping plane.")]]
	[[editor::min(0.00001f)]]
	[[editor::drag_speed(0.2)]]
	float far = 100.0f;

    [[editor::tooltip("Environment asset to render this camera's view with.")]]
    Handle<gfx::Environment> environment;
};

}