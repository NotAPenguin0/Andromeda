#ifndef ANDROMEDA_MESH_HPP_
#define ANDROMEDA_MESH_HPP_

#include <phobos/util/buffer_util.hpp>

namespace andromeda {

class Mesh {
public:
	ph::RawBuffer get_vertices() { return vertices; }
	ph::RawBuffer get_indices() { return indices; }
	uint32_t index_count() { return indices_size; }

	ph::RawBuffer vertices;
	ph::RawBuffer indices;
	uint32_t indices_size = 0;
};

}

#endif