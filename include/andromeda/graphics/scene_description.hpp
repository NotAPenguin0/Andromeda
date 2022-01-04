#pragma once

#include <andromeda/components/transform.hpp>
#include <andromeda/components/camera.hpp>
#include <andromeda/components/point_light.hpp>
#include <andromeda/components/directional_light.hpp>
#include <andromeda/components/postprocessing.hpp>
#include <andromeda/ecs/registry.hpp>
#include <andromeda/graphics/forward.hpp>
#include <andromeda/graphics/viewport.hpp>
#include <andromeda/thread/locked_value.hpp>
#include <andromeda/util/handle.hpp>

#include <phobos/image.hpp>

#include <glm/mat4x4.hpp>

#include <glsl/limits.glsl>
#include <glsl/types.glsl>

#include <array>
#include <span>
#include <vector>

namespace andromeda::gfx {

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
        uint32_t const normal = 0;
        uint32_t const metal_rough = 0;
        uint32_t const occlusion = 0;
	};

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
    };


    /**
     * @brief Stores a single viewport with its camera data.
     */
    struct CameraInfo {
        /**
         * @brief Projection, view and projection * view matrices for this camera.
         */
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 proj_view;

        /**
         * @brief Camera position
         */
        glm::vec3 position;

        /**
         * @brief Whether this camera is active. You will never have to draw viewports with inactive cameras.
         */
        bool active = false;

        /**
         * @brief Handle to the environment map. If this is null, either render an atmosphere or no environment.
         */
        Handle<gfx::Environment> environment;

        /**
         * @brief Exposure parameter. Defined as log2(min_luminance)
         */
        float min_log_luminance;

        /**
         * @brief Exposure parameter. defined as log2(max_luminance)
         */
        float max_log_luminance;
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
     * @brief Register a directional light.
     * @param light Reference to the DirectionalLight component.
     * @param rotation Rotation around the forward (0, 0, -1) axis of the entity. Used to compute light direction.
     */
    void add_light(DirectionalLight const& light, glm::vec3 const& rotation);

	/**
	 * @brief Adds a viewport + camera to the system.
	 * @param vp Viewport to add
	 * @param ecs Entity component system to retrieve camera data from
	 * @param camera Camera entity in the ECS
	*/
	void add_viewport(gfx::Viewport const& vp, thread::LockedValue<const ecs::registry> const& ecs, ecs::entity_t camera);

    /**
     * @brief Sets the default albedo texture. This will be used as a placeholder if no albedo texture was loaded.
     * @param handle Handle to the default albedo texture to use. This may not be a null handle.
    */
    void set_default_albedo(Handle<gfx::Texture> handle);

    /**
     * @brief Sets the default normal map. This will be used as a placeholder if no normal map was loaded.
     * @param handle Handle to the default normal map to use. This may not be a null handle.
    */
    void set_default_normal(Handle<gfx::Texture> handle);

    /**
     * @brief Sets the default metal/rough map. This will be used as a placeholder if no metallic map was loaded.
     * @param handle Handle to the default metallic map to use. This may not be a null handle.
    */
    void set_default_metal_rough(Handle<gfx::Texture> handle);

    /**
     * @brief Sets the default ambient occlusion map. This will be used as a placeholder if no ambient occlusion map was loaded.
     * @param handle Handle to the default ambient occlusion map to use. This may not be a null handle.
    */
    void set_default_occlusion(Handle<gfx::Texture> handle);

    /**
     * @brief Sets the default environment object. This can be used as a placeholder if no environment was loaded (yet).
     * @param handle Handle the the default environment object. May not be null.
     */
    void set_default_environment(Handle<gfx::Environment> handle);

    /**
     * @brief Sets the 2D BRDF lookup texture. Does not get cleared on reset()
     * @param handle Handle to the 2D BRDF lookup texture. If this is not loaded yet, a black 1x1 texture is used instead.
     */
    void set_brdf_lut(Handle<gfx::Texture> handle);

	/**
	 * @brief Clears all data except things that are persistent across frames (such as
	 *		  default textures).
	*/
	void reset();

    // Accessor functions

	/**
	 * @brief Get indices for reach texture in the material. The material must be added with 
	 *        add_material() before calling this.
	 * @param material Material to query textures for.
	 * @return An instance of MaterialTextures holding all indices.
	*/
	MaterialTextures get_material_textures(Handle<gfx::Material> material) const;

    /**
     * @brief Get a list of draws
     * @return Span over the range of all draws in the scene
     */
    std::span<Draw const> get_draws() const;

    /**
     * @brief Get a list of transforms. Each draw at index i has a transform matrix in this span at the same index i.
     * @return Span over the range of all transform matrices in the scene.
     */
    std::span<glm::mat4 const> get_draw_transforms() const;

    /**
     * @brief Get information for a camera associated with a certain viewport.s
     * @param vp Viewport to get the camera info for.
     * @return Structure containing all camera info.
     */
    CameraInfo const& get_camera_info(gfx::Viewport const& vp) const;

    /**
     * @brief Get a handle to the BRDF lookup texture.
     * @return Handle to the BRDF lookup texture.
     */
    Handle<gfx::Texture> get_brdf_lookup() const;

    /**
     * @brief Get a handle to an always-ready environment that can be used when no environment is loaded.
     * @return Handle to the default environment. This is always ready to use.
     */
    Handle<gfx::Environment> get_default_environment() const;

    /**
     * @brief Get a handle to the default albedo texture. This is always ready.
     * @return Handle to the default albedo texture (magenta).
     */
    Handle<gfx::Texture> get_default_albedo() const;

    /**
     * @brief Get a list of all textures that need to be in the texture descriptor array.
     * @return Span over all texture ImageViews.
     */
    std::span<ph::ImageView const> get_textures() const;

    /**
     * @brief Get a list of all point lights in the scene, in a GPU-ready format (defined in glsl/types.glsl)
     * @return Span over all point light structures.
     */
    std::span<gpu::PointLight const> get_point_lights() const;

    /**
     * @brief Get a list of all directional lights in the scene, in a GPU-ready format (defined in glsl/types.glsl)
     * @return Span over all directional light structures
     */
    std::span<gpu::DirectionalLight const> get_directional_lights() const;

private:

	/**
	 * @brief Stores all draws to process.
	*/
	std::vector<Draw> draws;
    /**
     * @brief Stores all draw transformation matrices. The reason we put this in a separate vector is
     *        so we can upload it to the GPU buffer in a single memcpy()
     */
    std::vector<glm::mat4> draw_transforms;

	// Each camera is indexed by a viewport index.
	std::array<CameraInfo, gfx::MAX_VIEWPORTS> cameras;

	struct {
		// Array of all image views ready to send to the GPU.
		std::vector<ph::ImageView> views;
		// Map texture ID to an index in the above list.
		mutable std::unordered_map<Handle<gfx::Texture>, uint32_t> id_to_index;

        // Default textures that can be set by using set_default_xxx(). These are not reset when calling reset().
        Handle<gfx::Texture> default_albedo;
        Handle<gfx::Texture> default_normal;
        Handle<gfx::Texture> default_metal_rough;
        Handle<gfx::Texture> default_occlusion;

        // BRDF lookup texture.
        Handle<gfx::Texture> brdf_lut;
	} textures;

    // Default environment. This environment has an empty irradiance and specular map, so they won't affect lighting.
    // The cubemap texture for this environment is transparent so that if you do render it nothing actually shows up.
    // Not reset when calling reset()
    Handle<gfx::Environment> default_env;

    std::vector<gpu::PointLight> point_lights;
    std::vector<gpu::DirectionalLight> directional_lights;
    // Amount of directional lights that are shadow casters
    // Will never be more than ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS in glsl/limits.glsl
    uint32_t num_shadowing_dir_lights = 0;

	/**
	 * @brief Adds an individual texture to the system.
	*/
	void add_texture(Handle<gfx::Texture> texture);
};

} // namespace andromeda::gfx