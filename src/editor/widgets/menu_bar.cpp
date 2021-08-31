#include <andromeda/editor/widgets/menu_bar.hpp>

namespace andromeda::editor {

MenuBar::MenuBuilder::MenuBuilder(bool visible) : visible(visible) {

}

MenuBar::MenuBuilder MenuBar::MenuBuilder::submenu(std::string const& name) {
	if (visible) {
		bool result = ImGui::BeginMenu(name.c_str());
		return MenuBuilder{ result };
	}
	// Not visible, return empty builder so future build operations down the 
	// hierarchy are also ignored.
	return MenuBuilder{ false };
}

void MenuBar::MenuBuilder::item(std::string const& name, bool& selected, std::string const& shortcut) {
	if (visible) {
		ImGui::MenuItem(name.c_str(), shortcut.c_str(), &selected);
	}
}

bool MenuBar::MenuBuilder::is_visible() const {
	return visible;
}


MenuBar::MenuBuilder::~MenuBuilder() {
	if (visible) {
		ImGui::EndMenu();
	}
}

MenuBar::MenuBar(bool main_menu) {
	if (main_menu) visible = ImGui::BeginMainMenuBar();
	else visible = ImGui::BeginMenuBar();

	is_main_menu = main_menu;
}

MenuBar::~MenuBar() {
	if (visible) {
		if (is_main_menu) {
			ImGui::EndMainMenuBar();
		}
		else {
			ImGui::EndMenuBar();
		}
	}
}

MenuBar::MenuBuilder MenuBar::submenu(std::string const& name) {
	bool result = ImGui::BeginMenu(name.c_str());
	return MenuBuilder{ result };
}

void MenuBar::item(std::string const& name, bool& selected, std::string const& shortcut) {
	ImGui::MenuItem(name.c_str(), shortcut.c_str(), &selected);
}

}