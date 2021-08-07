#pragma once

#include <andromeda/util/sparse_set.hpp>
#include <andromeda/ecs/entity.hpp>

namespace andromeda::ecs {

class component_storage_base : public sparse_set<entity_t> {
public:
    using sparse_set::sparse_set;
};

}
