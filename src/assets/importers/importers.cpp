#include <andromeda/assets/importers/importers.hpp>
#include <vulkan/vulkan.hpp>

namespace andromeda::assets::importers {

vk::Format get_vk_format(TextureInfo const& info) {
	switch (info.color_space) {
	case ColorSpace::Srgb: {
		switch (info.format) {
		case TextureFormat::rgb:
			return vk::Format::eR8G8B8Srgb;
		case TextureFormat::rgba:
			return vk::Format::eR8G8B8A8Srgb;
		case TextureFormat::greyscale:
			return vk::Format::eR8Srgb;
		case TextureFormat::grayscale_alpha:
			return vk::Format::eR8G8Srgb;
		}
	} break;
	case ColorSpace::Linear: {
		switch (info.format) {
		case TextureFormat::rgb:
			return vk::Format::eR8G8B8Unorm;
		case TextureFormat::rgba:
			return vk::Format::eR8G8B8A8Unorm;
		case TextureFormat::greyscale:
			return vk::Format::eR8Unorm;
		case TextureFormat::grayscale_alpha:
			return vk::Format::eR8G8Unorm;
		}
	} break;
	}
}

}