#pragma once

#include <andromeda/ecs/entity.hpp>

#include <vector>

namespace andromeda {

struct Hierarchy {
	ecs::entity_t parent = ecs::no_entity;
	std::vector<ecs::entity_t> children;
};

}