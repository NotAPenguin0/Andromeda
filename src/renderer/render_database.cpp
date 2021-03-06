#include <andromeda/renderer/render_database.hpp>
#include <andromeda/assets/assets.hpp>

#include <stl/assert.hpp>

namespace andromeda::renderer {

void RenderDatabase::add_material(Handle<Material> handle) {
	// Add all textures for this material to the database
	Material* material = assets::get(handle);
	STL_ASSERT(material, "Invalid material handle");
	add_texture(material->color);
	add_texture(material->normal);
	add_texture(material->metallic);
	add_texture(material->roughness);
	add_texture(material->ambient_occlusion);
}

void RenderDatabase::add_draw(Draw const& draw) {
	draws.push_back(InternalDraw{ .mesh = draw.mesh, .material = draw.material });
	transforms.push_back(draw.transform);
}

void RenderDatabase::add_point_light(glm::vec3 const& position, float radius, glm::vec3 const& color, float intensity) {
	point_lights.push_back(InternalPointLight{
		.position = position,
		.radius = radius,
		.color = color,
		.intensity = intensity
	});
}

void RenderDatabase::add_texture(Handle<Texture> handle) {
	// Skip empty texture handles, those are handled later using the default textures system
	if (handle.id == Handle<Texture>::none) { return; }
	if (!assets::is_ready(handle)) { return; }
	Texture* texture = assets::get(handle);
	// Specifying an invalid texture handle is an error though
	STL_ASSERT(texture, "Invalid texture handle");
	// Register this texture
	texture_views.push_back(texture->get_view());
	texture_map.emplace(handle.id, texture_views.size() - 1);
}

void RenderDatabase::reset() {
	texture_views.clear();
	draws.clear();
	transforms.clear();
	texture_map.clear();
	point_lights.clear();

	texture_views.resize(default_texture_count);
	texture_views[default_color_index] = default_color;
	texture_views[default_normal_index] = default_normal;
	texture_views[default_metallic_index] = default_metallic;
	texture_views[default_roughness_index] = default_roughness;
	texture_views[default_ambient_occlusion_index] = default_ambient_occlusion;
}

RenderDatabase::TextureIndices RenderDatabase::get_material_textures(Handle<Material> handle) {
	Material* material = assets::get(handle);
	STL_ASSERT(material, "Invalid material handle");
	return TextureIndices{
		.color = material->color ? texture_map[material->color.id] : default_color_index,
		.normal = material->normal ? texture_map[material->normal.id] : default_normal_index,
		.metallic = material->metallic ? texture_map[material->metallic.id] : default_metallic_index,
		.roughness = material->roughness ? texture_map[material->roughness.id] : default_roughness_index,
		.ambient_occlusion = material->ambient_occlusion ? texture_map[material->ambient_occlusion.id] : default_ambient_occlusion_index
	};
}

}