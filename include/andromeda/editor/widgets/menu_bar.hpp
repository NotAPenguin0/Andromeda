#pragma once

#include <andromeda/graphics/imgui.hpp>

namespace andromeda::editor {

/**
 * @class MenuBar 
 * @brief Utility class that serves as a RAII wrapper around ImGui::BeginMenuBar() and ImGui::EndMenuBar()
*/
class MenuBar {
public:
	/**
	 * @class MenuBuilder 
	 * @brief Helper used to build ImGui menus.
	*/
	class MenuBuilder {
	public:

		MenuBuilder(MenuBuilder const&) = delete;
		MenuBuilder& operator=(MenuBuilder const&) = delete;

		MenuBuilder(MenuBuilder&&) = default;
		MenuBuilder& operator=(MenuBuilder&&) = default;

		~MenuBuilder();

		/**
		 * @brief Create a submenu
		 * @param name A unique identifier to label the submenu.
		 * @return A MenuBuilder instance that when going out of scope will end the submenu context.
		 *		   This instance can be used to further build a deeper UI structure.
		*/
		MenuBuilder submenu(std::string const& name);

		/**
		 * @brief Create a menu item.
		 * @param name A unique identifier to label the menu item.
		 * @param selected Reference to a bool that will give back whether the menu is clicked or not.
		 *		  A pointer to this is stored internally so make sure the bool outlives the UI.
		 * @param shortcut Optionally a string to display a shortcut. Defaults to an empty string.
		*/
		void item(std::string const& name, bool& selected, std::string const& shortcut = "");

		/**
		 * @brief Query whether the submenu is visible.
		 * @return True if it is visible, false otherwise.
		*/
		bool is_visible() const;

	private:
		friend class MenuBar;

		/**
		 * @brief Create a new menu builder. 
		 * @param visible The return value of ImGui::BeginMenu
		*/
		MenuBuilder(bool visible);

		bool visible = false;
	};

	/**
	 * @brief Starts the menu bar context.
	 * @param main_menu Whether to create this menu bar as a main menu bar. Uses ImGui::BeginMainMenuBar() instead.
	*/
	MenuBar(bool main_menu = false);

	~MenuBar();

	/**
	 * @brief Create a submenu
	 * @param name A unique identifier to label the submenu.
	 * @return A MenuBuilder instance that when going out of scope will end the submenu context.
	 *		   This instance can be used to further build a deeper UI structure.
	*/
	MenuBuilder submenu(std::string const& name);

	/**
	 * @brief Create a menu item.
	 * @param name A unique identifier to label the menu item.
	 * @param selected Reference to a bool that will give back whether the menu is clicked or not.
	 *		  A pointer to this is stored internally so make sure the bool outlives the UI.
	 * @param shortcut Optionally a string to display a shortcut. Defaults to an empty string.
	*/
	void item(std::string const& name, bool& selected, std::string const& shortcut = "");

private:
	bool is_main_menu = false;
	bool visible = false;
};

}