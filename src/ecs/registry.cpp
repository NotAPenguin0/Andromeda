#include <andromeda/ecs/registry.hpp>

#include <tuple>

namespace andromeda::ecs {

namespace {

template<typename C>
struct component_copy {
    void operator()(registry const& src, registry& dst, entity_t src_entity, entity_t dst_entity) {
        // Do the copy
        if (src.has_component<C>(src_entity)) {
            dst.add_component<C>(dst_entity, src.get_component<C>(src_entity));
        }
    }
};

}

registry::registry() {

}

entity_t registry::create_entity() {
    entity_t id = id_generator.next();
    entities.push_back(id);
    return id;
}

entity_t registry::create_blueprint_entity() {
    entity_t entity = create_entity();
    // TODO: Implement blueprint entities
//    add_component<components::Blueprint>(entity, entity);
    return entity;
}

/*
entity_t registry::import_blueprint(registry& source, entity_t other) {
    auto copy_fun = [this, &source](entity_t src_entity, stl::tree<entity_t>::const_traverse_info info, entity_t parent) 
        -> stl::tuple<entity_t> {
        
        auto id = create_entity(parent);
        // do the copy
        meta::for_each_component<component_copy>(source, *this, src_entity, id);
        // Add a blueprint instance component referring to the old entity (= the blueprint)
        add_component<components::BlueprintInstance>(id, src_entity);

        return { id };
    };

    auto entity_it = source.get_entities().find(other);
    source.get_entities().traverse_from(entity_it, copy_fun, 0);

    // Return the latest created child of the root entity, aka the root of the entity that was just created
    return entities.root()->children.back().data;
}
*/

std::vector<entity_t> const& registry::get_entities() const {
    return entities;
}

} // namespace saturn::ecs