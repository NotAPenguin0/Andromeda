#ifndef ANDROMEDA_WORLD_HPP_
#define ANDROMEDA_WORLD_HPP_

#include <andromeda/components/hierarchy.hpp>
#include <andromeda/ecs/registry.hpp>
#include <mutex>

namespace andromeda::world {

class World {
public:
	World();

	// Returns the root entity of the world. Note that this entity should always have ID 0.
	ecs::entity_t root() const;

	ecs::registry& ecs();
	ecs::registry const& ecs() const;

	// Creates an entity with the required components.
	ecs::entity_t create_entity(ecs::entity_t parent = 0);
	
	components::Hierarchy& get_hierarchy(ecs::entity_t entity);
	components::Hierarchy const& get_hierarchy(ecs::entity_t entity) const;

	void lock();
	void unlock();

private:
	ecs::registry entities;
	// We will build the world from a root entity. This root entity should always have ID 0.
	ecs::entity_t root_entity = 0;

	std::mutex mutex;

	// Adds required components (Hierarchy and Transform) to an entity
	void initialize_entity(ecs::entity_t entity, ecs::entity_t parent);
};

}

#endif