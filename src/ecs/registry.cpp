#include <andromeda/ecs/registry.hpp>

#include <tuple>

namespace andromeda::ecs {

registry::registry() {

}

entity_t registry::create_entity() {
    entity_t id = id_generator.next();
    entities.push_back(id);
    return id;
}


std::vector<entity_t> const& registry::get_entities() const {
    return entities;
}

} // namespace saturn::ecs