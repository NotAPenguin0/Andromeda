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

    if (json.hasKey("albedo")) {
        if (json["albedo"].hasKey("path")) {
            material.albedo = assets::load<gfx::Texture>(json["albedo"]["path"].ToString());
        }
            // No albedo texture but base color value
        else if (json["albedo"].hasKey("color")) {
            auto const& color_json = json["albedo"]["color"];
            uint8_t values[4]{};
            for (int i = 0; i < 4; ++i) {
                values[i] = color_json.at(i).ToInt();
            }

            Handle<gfx::Texture> handle = assets::impl::insert_pending<gfx::Texture>();
            // We'll load this on the same thread since it's a fairly cheap operation
            load_1x1_texture(ctx, handle, values, thread);
            material.albedo = handle;
        }
    }
    if (json.hasKey("normal")) { material.normal = assets::load<gfx::Texture>(json["normal"]["path"].ToString()); }
    if (json.hasKey("metal_rough")) { material.metal_rough = assets::load<gfx::Texture>(json["metal_rough"]["path"].ToString()); }
    if (json.hasKey("occlusion")) { material.occlusion = assets::load<gfx::Texture>(json["occlusion"]["path"].ToString()); }

    assets::impl::make_ready(handle, std::move(material));
}

}
}