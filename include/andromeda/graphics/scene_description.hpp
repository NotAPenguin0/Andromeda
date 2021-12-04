#pragma once

#include <andromeda/components/transform.hpp>
#include <andromeda/components/camera.hpp>
#include <andromeda/components/point_light.hpp>
#include <andromeda/graphics/forward.hpp>
#include <andromeda/graphics/viewport.hpp>
#include <andromeda/util/handle.hpp>

#include <phobos/image.hpp>

#include <glm/mat4x4.hpp>

#include <glsl/limits.glsl>
#include <glsl/types.glsl>

#include <array>
#include <vector>

namespace andromeda::gfx {

namespace backend {
	class RendererBackend;
	class SimpleRenderer;
    class ForwardPlusRenderer;
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
	 * @brief Each field in this structure stores an index into the textures array.
	*/
	struct MaterialTextures {
		uint32_t const albedo = 0;
	};

	/**
	 * @brief Creates an empty scene description
	*/
	SceneDescription() = default;

	/**
	 * @brief Add a mesh to draw. 
	 * @param mesh Handle to the mesh to draw.
	 * @param material Handle to the material to draw this mesh with.
	 * @param transform Transformation matrix.
	*/
	void add_draw(Handle<gfx::Mesh> mesh, Handle<gfx::Material> material, glm::mat4 const& transform);

	/**
	 * @brief Register a material. All used materials must be added through this function.
	 * @param material Handle to the material to register.
	*/
	void add_material(Handle<gfx::Material> material);

    /**
     * @brief Register a point light.
     * @param light Reference to the PointLight component.
     * @param position World position of the light object.
     */
    void add_light(PointLight const& light, glm::vec3 const& position);

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

	/**
	 * @brief Get indices for reach texture in the material. The material must be added with 
	 *        add_material() before calling this.
	 * @param material Material to query textures for.
	 * @return An instance of MaterialTextures holding all indices.
	*/
	MaterialTextures get_material_textures(Handle<gfx::Material> material) const;

private:
	friend class backend::RendererBackend;
	friend class backend::SimpleRenderer;
    friend class backend::ForwardPlusRenderer;

	/**
	 * @brief Describes a single draw command.
	*/
	struct Draw {
		/**
		 * @brief Handle to the mesh to draw.
		*/
		Handle<gfx::Mesh> mesh;

		/**
		 * @brief Handle to the material to be used for drawing this mesh.
		*/
		Handle<gfx::Material> material;

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

	struct {
		// Array of all image views ready to send to the GPU.
		std::vector<ph::ImageView> views;
		// Map texture ID to an index in the above list.
		mutable std::unordered_map<Handle<gfx::Texture>, uint32_t> id_to_index;
	} textures;

    std::vector<gpu::PointLight> point_lights;

	/**
	 * @brief Adds an individual texture to the system.
	*/
	void add_texture(Handle<gfx::Texture> texture);
};

} // namespace andromeda::gfx