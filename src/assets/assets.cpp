#include <andromeda/assets/assets.hpp>

namespace andromeda {

class Texture;

namespace assets {

template<>
Handle<Texture> load(std::string_view path) {

}

}
}