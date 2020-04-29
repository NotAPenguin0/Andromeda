#ifndef ANDROMEDA_ECS_ENTITY_HPP_
#define ANDROMEDA_ECS_ENTITY_HPP_

#include <andromeda/util/types.hpp>

namespace andromeda::ecs {

using entity_t = std::uint64_t;
constexpr inline entity_t no_entity = static_cast<entity_t>(-1);

}

#endif