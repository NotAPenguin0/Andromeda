#pragma once

#include <andromeda/graphics/imgui.hpp>

#include <andromeda/editor/style.hpp>

#include <functional>
#include <string>

namespace andromeda::editor {

/**
 * @class FloatingPanel 
 * @brief This class represents a floating panel over a window in a fixed position.
*/
class FloatingPanel {
public:
	/**
	 * @brief Create a floating panel.
	 * @param name Unique string ID of the panel. This is not shown in the UI.
	 * @param position Position of the panel, relative to the parent window.
	 * @param size Size of the panel, in pixels.
	 * @param alpha Opacity of the panel, default value is 1.0
	*/
	FloatingPanel(std::string const& name, ImVec2 position, ImVec2 size, float alpha = 1.0f);

	/**
	 * @brief Displays the panel, if it is visible.
	 * @param func Put ImGui widgets inside this function to render them inside the panel.
	*/
	void display(std::function<void()> func);

	~FloatingPanel();

private:
	bool begin_result = false;

	style::ScopedStyleVar<ImVec2> padding;
	style::ScopedStyleColor bg_color;
};

} // namespace andromeda::editor