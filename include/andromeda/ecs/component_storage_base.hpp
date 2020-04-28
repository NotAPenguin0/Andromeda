#ifndef ANDROMEDA_ECS_COMPONENT_STORAGE_BASE_HPP_
#define ANDROMEDA_ECS_COMPONENT_STORAGE_BASE_HPP_

#include <andromeda/util/sparse_set.hpp>
#include <andromeda/ecs/entity.hpp>

namespace andromeda::ecs {

class component_storage_base : public util::sparse_set<entity_t> {
public:
    using sparse_set::sparse_set;
};

}

#endif