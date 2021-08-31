#include <andromeda/graphics/scene_description.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace andromeda::gfx {

void SceneDescription::add_draw(Handle<gfx::Mesh> mesh, glm::mat4 const& transform) {
	draws.emplace_back(mesh, transform);
}

void SceneDescription::add_viewport(gfx::Viewport const& vp, Transform const& cam_transform, Camera const& cam) {
	CameraMatrices& camera = cameras[vp.index()];
	camera.active = true;

	// Projection matrix

	float aspect = (float)vp.width() / (float)vp.height();
	camera.projection = glm::perspective(glm::radians(cam.fov), aspect, cam.near, cam.far);
	// Vulkan needs upside down projection matrix
	camera.projection[1][1] *= -1;

	// View matrix

	float const cos_pitch = std::cos(glm::radians(cam_transform.rotation.x));
	float const cos_yaw = std::cos(glm::radians(cam_transform.rotation.y));
	float const sin_pitch = std::sin(glm::radians(cam_transform.rotation.x));
	float const sin_yaw = std::sin(glm::radians(cam_transform.rotation.y));

	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;

	front.x = cos_pitch * cos_yaw;
	front.y = sin_pitch;
	front.z = cos_pitch * sin_yaw;
	front = glm::normalize(front);
	right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
	up = glm::normalize(glm::cross(right, front));
	camera.view = glm::lookAt(cam_transform.position, cam_transform.position + front, up);

	// Precompute projection-view matrix as its commonly used.
	camera.proj_view = camera.projection * camera.view;
}

void SceneDescription::reset() {
	draws.clear();

	for (auto& cam : cameras) {
		cam.active = false;
	}
}

}