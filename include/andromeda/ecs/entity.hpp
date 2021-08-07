#pragma once

#include <cstdint>

namespace andromeda::ecs {

using entity_t = std::uint64_t;
constexpr inline entity_t no_entity = static_cast<entity_t>(-1);

}
