#pragma once

#include <andromeda/components/transform.hpp>
#include <andromeda/components/camera.hpp>
#include <andromeda/graphics/forward.hpp>
#include <andromeda/graphics/viewport.hpp>
#include <andromeda/util/handle.hpp>

#include <glm/mat4x4.hpp>

#include <array>
#include <vector>

namespace andromeda::gfx {

namespace backend {
	class RendererBackend;
	class SimpleRenderer;
}

/**
 * @class SceneDescription 
 * @brief Stores all information needed to render the scene in a format
 *		  that's easier to use for rendering backends. The entities in the world
 *		  will be added to this class and then sent to the rendering backend.
*/
class SceneDescription {
public:
	/**
	 * @brief Creates an empty scene description
	*/
	SceneDescription() = default;

	/**
	 * @brief Add a mesh to draw. 
	 * @param mesh Handle to the mesh to draw.
	 * @param transform Transformation matrix.
	*/
	void add_draw(Handle<gfx::Mesh> mesh, glm::mat4 const& transform);

	/**
	 * @brief Adds a viewport + camera to the system.
	 * @param vp Viewport to add
	 * @param cam_transform Transform component of the camera entity
	 * @param cam Camera component of the camera entity;
	*/
	void add_viewport(gfx::Viewport const& vp, Transform const& cam_transform, Camera const &cam);

	/**
	 * @brief Clears all data except things that are persistent across frames (such as
	 *		  default textures).
	*/
	void reset();

private:
	friend class backend::RendererBackend;
	friend class backend::SimpleRenderer;

	/**
	 * @brief Describes a single draw command.
	*/
	struct Draw {
		/**
		 * @brief Handle to the mesh to draw.
		*/
		Handle<gfx::Mesh> mesh;
		/**
		 * @brief Transformation matrix.
		*/
		glm::mat4 transform;
	};

	/**
	 * @brief Stores all draws to process.
	*/
	std::vector<Draw> draws;

	/**
	 * @brief Stores a single viewport + camera.
	*/
	struct CameraMatrices {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 proj_view;

		bool active = false;
	};

	// Each camera is indexed by a viewport index.
	std::array<CameraMatrices, gfx::MAX_VIEWPORTS> cameras;
};

} // namespace andromeda::gfx