#pragma once

#include <cstdint>

namespace andromeda::ecs {

struct type_id_counter {
    static inline uint64_t cur = 0;

    static uint64_t next() {
        return cur++;
    }
};

template<typename T>
uint64_t get_component_type_id() {
    static uint64_t id = type_id_counter::next();
    return id;
}


} // namespace andromeda::ecs
