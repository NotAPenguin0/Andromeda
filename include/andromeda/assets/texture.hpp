#ifndef ANDROMEDA_TEXTURE_HPP_
#define ANDROMEDA_TEXTURE_HPP_

#include <phobos/util/image_util.hpp>

namespace andromeda {

class Texture {
public:
	ph::RawImage get_image() { return image; }
	ph::ImageView get_view() { return view; }

	ph::RawImage image;
	ph::ImageView view;
};

}

#endif