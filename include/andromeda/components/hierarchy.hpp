#ifndef ANDROMEDA_HIERARCHY_COMPONENT_HPP_
#define ANDROMEDA_HIERARCHY_COMPONENT_HPP_

#include <vector>
#include <andromeda/ecs/entity.hpp>

namespace andromeda::components {

struct [[component]] Hierarchy {
	ecs::entity_t parent = 0;
	std::vector<ecs::entity_t> children;
};

}

#endif