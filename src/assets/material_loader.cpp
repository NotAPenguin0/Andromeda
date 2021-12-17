#include <andromeda/assets/loaders.hpp>

#include <andromeda/graphics/context.hpp>
#include <andromeda/graphics/material.hpp>

#include <plib/stream.hpp>
#include <json/json.hpp>

namespace andromeda {
namespace impl {

void load_material(gfx::Context& ctx, Handle<gfx::Material> handle, std::string_view path, uint32_t thread) {
	gfx::Material material{};

	plib::binary_input_stream file = plib::binary_input_stream::from_file(path.data());

	std::string json_string{};
	json_string.resize(file.size());
	file.read<char>(json_string.data(), file.size());

	json::JSON json = json::JSON::Load(json_string);

    if (json.hasKey("albedo")) material.albedo = assets::load<gfx::Texture>(ctx, json["albedo"].ToString());
    if (json.hasKey("normal")) material.normal = assets::load<gfx::Texture>(ctx, json["normal"].ToString());
    if (json.hasKey("metallic")) material.metallic = assets::load<gfx::Texture>(ctx, json["metallic"].ToString());
    if (json.hasKey("roughness")) material.roughness = assets::load<gfx::Texture>(ctx, json["roughness"].ToString());
    if (json.hasKey("occlusion")) material.occlusion = assets::load<gfx::Texture>(ctx, json["occlusion"].ToString());

	assets::impl::make_ready(handle, std::move(material));
}

}
}