#ifndef ANDROMEDA_TEXTURE_HPP_
#define ANDROMEDA_TEXTURE_HPP_

#include <andromeda/core/context.hpp>
#include <phobos/util/image_util.hpp>

namespace andromeda {

namespace renderer {
class Renderer;
class RenderDatabase;
}


class Texture {
public:

private:
	friend class Context;
	friend class renderer::Renderer;
	friend class renderer::RenderDatabase;

	ph::RawImage image;
	ph::ImageView view;
};

}

#endif