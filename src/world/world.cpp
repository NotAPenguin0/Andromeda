#include <andromeda/world/world.hpp>

#include <andromeda/components/hierarchy.hpp>
#include <andromeda/components/transform.hpp>

namespace andromeda::world {

World::World() {
	root_entity = entities.create_entity();
	initialize_entity(root_entity, ecs::no_entity);
}

ecs::entity_t World::root() const { return root_entity; }

ecs::registry& World::ecs() { return entities; }
ecs::registry const& World::ecs() const { return entities; }

ecs::entity_t World::create_entity(ecs::entity_t parent) {
	assert(parent != ecs::no_entity && "All entities must be children of the root entity (directly or indirectly)");
	ecs::entity_t entity = entities.create_entity();
	initialize_entity(entity, parent);
	return entity;
}

void World::initialize_entity(ecs::entity_t entity, ecs::entity_t parent) {
	using namespace components;

	Hierarchy& hierarchy = entities.add_component<Hierarchy>(entity);
	hierarchy.parent = parent;
	// If this entity is not the root entity, we have to update the parent's children
	if (parent != ecs::no_entity) {
		Hierarchy& parent_hierarchy = entities.get_component<Hierarchy>(parent);
		parent_hierarchy.children.push_back(entity);
	}

	entities.add_component<Transform>(entity);
}

components::Hierarchy& World::get_hierarchy(ecs::entity_t entity) {
	return entities.get_component<components::Hierarchy>(entity);
}

components::Hierarchy const& World::get_hierarchy(ecs::entity_t entity) const {
	return entities.get_component<components::Hierarchy>(entity);
}

}