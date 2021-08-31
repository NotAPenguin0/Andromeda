#pragma once

#include <andromeda/graphics/imgui.hpp>

#include <andromeda/world.hpp>

namespace andromeda::editor {

/**
 * @brief This widget is responsible for the entity/component inspector.
*/
class Inspector {
public:

	/**
	 * @brief Displays the inspector UI, if visible.
	 * @param world Reference to the world being inspected.
	*/
	void display(World& world);

	/**
	 * @brief Obtain a reference to the visible variable. Useful if you want to control visibility of this widget from menus for example.
	 * @return Reference to the visible flag. Lives as long as the inspector lives.
	*/
	bool& is_visible();

	/**
	 * @brief Obtain a reference to the currently selected entity.
	 * @return The selected entity.
	*/
	ecs::entity_t& selected();

	/**
	 * @brief Obtain a reference to the currently selected entity.
	 * @return The selected entity.
	*/
	ecs::entity_t const& selected() const;

private:
	bool visible = true;

	ecs::entity_t selected_entity = ecs::no_entity;

	void show_entity_list(World& world);

	// Show a tree item for this entity, and recursively for its children.
	void show_entity_tree_item(thread::LockedValue<ecs::registry>& ecs, ecs::entity_t entity);

	void show_details_panel(World& world);
};

}