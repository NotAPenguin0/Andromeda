#ifndef ANDROMEDA_RENDERER_RENDER_DATABASE_HPP_
#define ANDROMEDA_RENDERER_RENDER_DATABASE_HPP_

#include <andromeda/util/handle.hpp>
#include <andromeda/assets/material.hpp>
#include <andromeda/assets/texture.hpp>
#include <andromeda/assets/mesh.hpp>

#include <phobos/util/image_util.hpp>
#include <phobos/renderer/meta.hpp>

#include <array>
#include <unordered_map>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace andromeda {
namespace renderer {

struct Draw {
	Handle<Mesh> mesh;
	Handle<Material> material;
	glm::mat4 transform;
};

// This class stores all data to render in a format that is easier to use by the renderer
class RenderDatabase {
public:
	// Adds a material to the render database. All used materials in the drawcalls must be registered with this function in order to be valid.
	// Not registering a material may result in wrong textures being used, or even application crashes.
	void add_material(Handle<Material> handle);
	void add_draw(Draw const& draw);
	void add_point_light(glm::vec3 const& position, float radius, glm::vec3 const& color, float intensity);

	void reset();

	struct TextureIndices {
		uint32_t diffuse = 0;
		uint32_t normal = 0; // TODO: Abuse these default values to handle default textures?
	};

	TextureIndices get_material_textures(Handle<Material> handle);

	// These are not meant for modifying manually. Use the provided add_X functions for that. The only reason these are public is because
	// you can't friend a lambda and getters are ugly.

	// Stores all ImageViews ready to be sent to the shader easily. This is populated when a material is registered.
	std::vector<ph::ImageView> texture_views;
	// Mapping for Handle<Texture> => Index into texture_views array. This is necessary due to the way ID's are generated, we can't simply 
	// index into the texture_views array by ID. Suppose you allocate textures with IDs 0, 1, 2, and 3. Now you delete textures 0 and 1.
	// When pushing the textures into the array, they will get indices 0 and 1, but their IDs are 2 and 3. Maintaining a correct mapping is
	// the only real way to get around this while still guaranteeing contiguous storage for the ImageViews
	std::unordered_map<uint32_t, uint32_t> texture_map;
	struct InternalDraw {
		Handle<Mesh> mesh;
		Handle<Material> material;
	};
	std::vector<InternalDraw> draws;
	std::vector<glm::mat4> transforms;

	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 projection_view;
	glm::vec3 camera_position;

	// Stores point lights in a format ready to send to the shader (all data packed together)
	struct alignas(2 * sizeof(glm::vec4)) InternalPointLight {
		glm::vec3 position;
		float radius;
		glm::vec3 color;
		float intensity;
	};
	std::vector<InternalPointLight> point_lights;

private:
	void add_texture(Handle<Texture> handle);
};

}
}

#endif