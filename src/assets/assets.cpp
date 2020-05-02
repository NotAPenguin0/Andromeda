#include <andromeda/assets/assets.hpp>

#include <andromeda/assets/importers/png.hpp>
#include <andromeda/assets/importers/stb_image.h>
#include <chrono>
namespace ch = std::chrono;

#include <vulkan/vulkan.hpp>

#include <andromeda/util/log.hpp>
#include <fmt/chrono.h>

namespace andromeda {

class Texture;

namespace assets {

template<>
Handle<Texture> load(Context& ctx, std::string_view path) {
	auto start = ch::system_clock::now();
	
	int width, height, channels;
	// Always load image as rgba
	importers::byte* data = stbi_load(path.data(), &width, &height, &channels, 4);

	auto end = ch::system_clock::now();
	io::log("PNG with size {}x{} loaded in {} ms", width, height, 
		ch::duration_cast<ch::milliseconds>(end - start).count());

	auto handle = ctx.request_texture(width, height, vk::Format::eR8G8B8A8Srgb, data);
	stbi_image_free(data);
	return handle;
}

}
}