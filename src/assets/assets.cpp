#include <andromeda/assets/assets.hpp>

#include <andromeda/assets/importers/png.hpp>
#include <vector>

namespace andromeda {

class Texture;

namespace assets {

template<>
Handle<Texture> load(Context& ctx, std::string_view path) {
	importers::OpenTexture texture = importers::png::open_file(path);
	uint32_t const size = importers::png::get_required_size(texture);

	std::vector<importers::byte> data(size);
	importers::png::load_texture(texture, data.data());

	vk::Format const format = importers::get_vk_format(texture.info);

	return ctx.request_texture(texture.info.width, texture.info.height, format, data.data());
}

}
}