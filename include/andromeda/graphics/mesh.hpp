#pragma once

#include <phobos/buffer.hpp>

namespace andromeda {
namespace gfx {

struct Mesh {
    ph::RawBuffer vertices;
    ph::RawBuffer indices;

    uint32_t num_vertices = 0;
    uint32_t num_indices = 0;
};

} // namespace gfx
} // namespace andromeda