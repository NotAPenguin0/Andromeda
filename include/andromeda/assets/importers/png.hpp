#ifndef ANDROMEDA_PNG_IMPORTER_HPP_
#define ANDROMEDA_PNG_IMPORTER_HPP_

#include <string_view>

#include <andromeda/assets/importers/importers.hpp>

namespace andromeda {
namespace assets {
namespace importers {
namespace png {

OpenTexture open_file(std::string_view path);
uint32_t get_byte_size(OpenTexture const& tex);
void load_texture(OpenTexture tex, byte* data);

}
}
}
}

#endif