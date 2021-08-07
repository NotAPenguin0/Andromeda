#pragma once

#include <andromeda/ecs/registry.hpp>
#include <andromeda/thread/locked_value.hpp>

namespace andromeda {

/**
 * @class World 
 * @brief Stores all entities in the scene.
*/
class World {
public:
	/**
	 * @brief Creates the world with a single root entity.
	*/
	World();

	/**
	 * @brief Get the root entity of the world. This is a dummy entity with ID 0.
	 * @return The root entity.
	*/
	ecs::entity_t root() const;

	/**
	 * @brief Get access to the internal ECS to create and manage entities.
	 * @return A thread-safe structure holding the ECS and a lock.
	*/
	thread::LockedValue<ecs::registry> ecs();

	/**
	 * @brief Get access to the internal ECS to create and manage entities.
	 * @return A thread-safe structure holding the ECS and a lock.
	*/
	thread::LockedValue<ecs::registry const> ecs() const;

	/**
	 * @brief Creates a new entity with all necessary components
	 * @param parent The parent entity. Default value is the root entity.
	 * @return The newly created entity;
	*/
	ecs::entity_t create_entity(ecs::entity_t parent = 0);

private:
	mutable std::mutex mutex;

	ecs::registry entities;
	ecs::entity_t root_entity = 0;

	/**
	 * @brief Adds the required components to an entity. This function is NOT thread safe and must be
	 *		  externally synchronized.
	 * @param entity The entity to initialize.
	 * @param parent The parent entity.
	*/
	void initialize_entity(ecs::entity_t entity, ecs::entity_t parent);
};

}