#pragma once

#include <andromeda/ecs/entity.hpp>

#include <vector>

namespace andromeda {

struct [[component, editor::hide]] Hierarchy {
    ecs::entity_t this_entity = ecs::no_entity; // May not be modified.

    ecs::entity_t parent = ecs::no_entity;
    std::vector<ecs::entity_t> children;
};

}