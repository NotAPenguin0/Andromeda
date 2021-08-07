#include <andromeda/world.hpp>

#include <andromeda/components/hierarchy.hpp>

namespace andromeda {

World::World() {
	root_entity = entities.create_entity();
	initialize_entity(root_entity, ecs::no_entity);
}

ecs::entity_t World::root() const {
	return root_entity;
}

thread::LockedValue<ecs::registry> World::ecs() {
	return { ._lock = std::lock_guard{mutex}, .value = entities };
}

thread::LockedValue<ecs::registry const> World::ecs() const {
	return { ._lock = std::lock_guard{mutex}, .value = entities };
}

ecs::entity_t World::create_entity(ecs::entity_t parent) {
	// Gain thread-safe access to the ECS.
	auto[_, ecs] = this->ecs();
	
	ecs::entity_t entity = ecs.create_entity();
	initialize_entity(entity, parent);
	return entity;
}

void World::initialize_entity(ecs::entity_t entity, ecs::entity_t parent) {
	Hierarchy& hierarchy = entities.add_component<Hierarchy>(entity);
	hierarchy.parent = parent;

	// If this entity is not the root entity, update the children member of the parent
	if (entity != root()) {
		Hierarchy& parent_hierarchy = entities.get_component<Hierarchy>(parent);
		parent_hierarchy.children.push_back(entity);
	}
}

}