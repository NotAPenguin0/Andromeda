#include <andromeda/world.hpp>

#include <andromeda/components/hierarchy.hpp>
#include <andromeda/components/transform.hpp>
#include <andromeda/components/name.hpp>
#include <reflect/reflection.hpp>

namespace andromeda {

namespace detail {
    template<typename C>
    struct component_copy {
        void operator()(ecs::registry const& src, ecs::registry& dst, ecs::entity_t src_entity, ecs::entity_t dst_entity) {
            if constexpr (std::is_same_v<C, Hierarchy>) return;
            // Do the copy
            if (src.has_component<C>(src_entity)) {
                if (dst.has_component<C>(dst_entity)) { // If already present in dst, simply copy
                    dst.get_component<C>(dst_entity) = src.get_component<C>(src_entity);
                } else {
                    dst.add_component<C>(dst_entity, src.get_component<C>(src_entity));
                }
            }
        }
    };
}

World::World() {
	root_entity = entities.create_entity();
	initialize_entity(root_entity, ecs::no_entity);

    blueprint_root = blueprint_entities.create_entity();
    initialize_blueprint(blueprint_root, ecs::no_entity);
}

ecs::entity_t World::root() const {
	return root_entity;
}

thread::LockedValue<ecs::registry> World::ecs() {
	return { ._lock = std::lock_guard{ mutex }, .value = entities };
}

thread::LockedValue<ecs::registry const> World::ecs() const {
	return { ._lock = std::lock_guard{ mutex }, .value = entities };
}

thread::LockedValue<ecs::registry> World::blueprints() {
    return { ._lock = std::lock_guard{ blueprint_mutex }, .value = blueprint_entities };
}

thread::LockedValue<ecs::registry const> World::blueprints() const {
    return { ._lock = std::lock_guard{ blueprint_mutex }, .value = blueprint_entities };
}

ecs::entity_t World::create_entity(ecs::entity_t parent) {
	// Gain thread-safe access to the ECS.
	auto lock = this->ecs();
	return create_entity(lock, parent);
}

ecs::entity_t World::create_entity(thread::LockedValue<ecs::registry>& locked_ecs, ecs::entity_t parent) {
    ecs::entity_t entity = locked_ecs->create_entity();
    initialize_entity(entity, parent);
    return entity;
}

ecs::entity_t World::create_blueprint(ecs::entity_t parent) {
    auto lock = this->blueprints();
    return create_blueprint(lock, parent);
}

ecs::entity_t World::create_blueprint(thread::LockedValue<ecs::registry>& locked_bp, ecs::entity_t parent) {
    ecs::entity_t entity = locked_bp->create_entity();
    initialize_blueprint(entity, parent);
    return entity;
}

static ecs::entity_t import_entity_impl(World* world, thread::LockedValue<ecs::registry>& ecs, thread::LockedValue<ecs::registry>& blueprints, ecs::entity_t src, ecs::entity_t parent) {
    ecs::entity_t dst = world->create_entity(ecs, parent);
    // Copy over all components
    meta::for_each_component<detail::component_copy>(blueprints.value, ecs.value, src, dst);
    {
        // Update hierarchy values
        auto &hierarchy = ecs->get_component<Hierarchy>(dst);
        hierarchy.this_entity = dst;
        hierarchy.children.clear(); // clear old children out of it
    }
    // Import children and update hierarchy alongside it
    auto& src_hierarchy = blueprints->get_component<Hierarchy>(src);
    for (auto child : src_hierarchy.children) {
        import_entity_impl(world, ecs, blueprints, child, dst);
    }
    // set new name for imported entity
    ecs->get_component<Name>(dst).name = "Instance of " + blueprints->get_component<Name>(src).name + "(" + std::to_string(dst) + ")";

    return dst;
}

ecs::entity_t World::import_entity(ecs::entity_t entity, ecs::entity_t parent) {
    auto bp = this->blueprints();
    auto ecs = this->ecs();

    return import_entity_impl(this, ecs, bp, entity, parent);
}

void World::initialize_entity(ecs::entity_t entity, ecs::entity_t parent) {
	auto& hierarchy = entities.add_component<Hierarchy>(entity);
	hierarchy.parent = parent;
	hierarchy.this_entity = entity;

	// If this entity is not the root entity, update the children member of the parent
	if (entity != root()) {
		auto& parent_hierarchy = entities.get_component<Hierarchy>(parent);
		parent_hierarchy.children.push_back(entity);
	}

	// Additionally, add an identity transform to every constructed entity.
	entities.add_component<Transform>(entity);
	entities.add_component<Name>(entity).name = "Entity " + std::to_string(entity);
}

void World::initialize_blueprint(ecs::entity_t entity, ecs::entity_t parent) {
    auto& hierarchy = blueprint_entities.add_component<Hierarchy>(entity);
    hierarchy.parent = parent;
    hierarchy.this_entity = entity;

    // If this entity is not the root entity, update the children member of the parent
    if (entity != root()) {
        auto& parent_hierarchy = blueprint_entities.get_component<Hierarchy>(parent);
        parent_hierarchy.children.push_back(entity);
    }

    blueprint_entities.add_component<Transform>(entity);
    blueprint_entities.add_component<Name>(entity).name = "Blueprint " + std::to_string(entity);
}

}