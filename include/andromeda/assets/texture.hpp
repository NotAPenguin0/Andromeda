#ifndef ANDROMEDA_TEXTURE_HPP_
#define ANDROMEDA_TEXTURE_HPP_

#include <andromeda/core/context.hpp>
#include <phobos/util/image_util.hpp>

namespace andromeda {

class Texture {
public:

private:
	friend class Context;

	ph::RawImage image;
	ph::ImageView view;
};

}

#endif