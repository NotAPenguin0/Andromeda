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
	
	if (!json.hasKey("albedo")) {
		LOG_FORMAT(LogLevel::Error, "Error while loading material at path {}: Required key 'albedo' not present.", path);
		return;
	}
	std::string albedo_path = json["albedo"].ToString();

	material.albedo = assets::load<gfx::Texture>(ctx, albedo_path);

	assets::impl::make_ready(handle, std::move(material));
}

}
}