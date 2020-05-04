#include <andromeda/renderer/render_database.hpp>
#include <andromeda/assets/assets.hpp>

#include <stl/assert.hpp>

namespace andromeda::renderer {

void RenderDatabase::add_material(Handle<Material> handle) {
	// Add all textures for this material to the database
	Material* material = assets::get(handle);
	STL_ASSERT(material, "Invalid material handle");
	add_texture(material->diffuse);
}

void RenderDatabase::add_draw(Draw const& draw) {
	draws.push_back(InternalDraw{ .mesh = draw.mesh, .material = draw.material });
	transforms.push_back(draw.transform);
}

void RenderDatabase::add_texture(Handle<Texture> handle) {
	// Skip empty texture handles, those are handled later using the default textures system
	if (handle.id == Handle<Texture>::none) { return; }
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
}

RenderDatabase::TextureIndices RenderDatabase::get_material_textures(Handle<Material> handle) {
	Material* material = assets::get(handle);
	STL_ASSERT(material, "Invalid material handle");
	return TextureIndices{
		.diffuse = texture_map[material->diffuse.id]
	};
}

}