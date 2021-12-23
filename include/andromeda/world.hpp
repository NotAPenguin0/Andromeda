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
     * @brief Get access to the internal ECS storing the blueprint entities.
     * @return A thread-safe structure holding the ECS and a lock.
     */
    thread::LockedValue<ecs::registry> blueprints();

    /**
     * @brief Get access to the internal ECS storing the blueprint entities.
     * @return A thread-safe structure holding the ECS and a lock.
     */
    thread::LockedValue<ecs::registry const> blueprints() const;

	/**
	 * @brief Creates a new entity with all necessary components
	 * @param parent The parent entity. Default value is the root entity.
	 * @return The newly created entity.
	*/
	ecs::entity_t create_entity(ecs::entity_t parent = 0);

    /**
     * @brief Creates a new entity with all necessary components. Use this overload if you already have thread-safe access to the ECS;
     * @param locked_ecs Reference to a thread-safe structure holding the ECS to manipulate
     * @param parent The parent entity. Default value is the root entity.
     * @return The newly created entity.
     */
    ecs::entity_t create_entity(thread::LockedValue<ecs::registry>& locked_ecs, ecs::entity_t parent = 0);

    /**
     * @brief Creates a new blueprint entity with all necessary components
     * @param parent Optionally a parent entity. Default value is the root entity.
     * @return The newly created entity.
     */
    ecs::entity_t create_blueprint(ecs::entity_t parent = 0);

    /**
     * @brief Creates a new blueprint entity with all necessary components. Use this overload if you already have thread-safe access to the ECS;
     * @param locked_ecs Reference to a thread-safe structure holding the ECS to manipulate
     * @param parent The parent entity. Default value is the root entity.
     * @return The newly created entity.
     */
    ecs::entity_t create_blueprint(thread::LockedValue<ecs::registry>& locked_bp, ecs::entity_t parent = 0);

    /**
     * @brief Imports an entity from the blueprint entity system.
     * @param entity Entity handle. This must be a valid entity handle coming from the blueprint system.
     * @param parent Optionally a handle to the parent in the new entity system.
     * @return Root entity of the imported entity;
     */
    ecs::entity_t import_entity(ecs::entity_t entity, ecs::entity_t parent = 0);

private:
	mutable std::mutex mutex;
    mutable std::mutex blueprint_mutex;

	ecs::registry entities;
    ecs::registry blueprint_entities;
	ecs::entity_t root_entity = 0;
    ecs::entity_t blueprint_root = 0;

	/**
	 * @brief Adds the required components to an entity. This function is NOT thread safe and must be
	 *		  externally synchronized.
	 * @param entity The entity to initialize.
	 * @param parent The parent entity.
	*/
	void initialize_entity(ecs::entity_t entity, ecs::entity_t parent);

    /**
     * @brief Initializes a blueprint entity. This function is NOT thread safe and must be externally synchronized.
     * @param entity The entity to initialize.
     * @param parent The parent entity.
     */
    void initialize_blueprint(ecs::entity_t entity, ecs::entity_t parent);
};

}