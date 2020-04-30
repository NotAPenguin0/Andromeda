#ifndef ANDROMEDA_ASSET_IMPORTERS_HPP_
#define ANDROMEDA_ASSET_IMPORTERS_HPP_

#include <andromeda/util/types.hpp>
#include <andromeda/util/vk_forward.hpp>

namespace andromeda {
namespace assets {
namespace importers {

using byte = uint8_t;

enum class TextureFormat {
	rgb,
	rgba,
	greyscale,
	grayscale_alpha
};

enum class ColorSpace {
	Srgb,
	Linear
};

struct TextureInfo {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t byte_width = 0;
	TextureFormat format = TextureFormat::rgb;
	ColorSpace color_space = ColorSpace::Linear;
};

struct OpenTexture {
	TextureInfo info;
	bool valid = true;
	void* file = nullptr;
	void* internal_data = nullptr;
};

vk::Format get_vk_format(TextureInfo const& info);

}
}
}

#endif