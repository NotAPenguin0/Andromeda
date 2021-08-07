#pragma once

#include <phobos/image.hpp>

namespace andromeda {
namespace gfx {

struct Texture {
	ph::RawImage image;
	ph::ImageView view;
};

} // namespace gfx
} // namespace andromeda