#pragma once

#include <andromeda/graphics/imgui.hpp>

#include <concepts>

namespace andromeda::editor::style {

void set_rounding(float rounding);
void color_preset_gray();

// Get the default style. Useful to temporarily revert style vars.
ImGuiStyle const& default_style();

/**
 * @struct ScopedStyleVar 
 * @brief Pushes a style variable onto the stack and pops it when going out of scope.
 * @tparam T Type of the style variable. Only types accepted are ImVec2 and float.
*/
template<typename T> requires std::same_as<T, ImVec2> || std::same_as<T, float>
struct ScopedStyleVar {
	[[nodiscard]] ScopedStyleVar(ImGuiStyleVar var, T value) {
		ImGui::PushStyleVar(var, value);
	}

	ScopedStyleVar(ScopedStyleVar const&) = delete;
	ScopedStyleVar(ScopedStyleVar&&) = delete;

	ScopedStyleVar& operator=(ScopedStyleVar const&) = delete;
	ScopedStyleVar& operator=(ScopedStyleVar&&) = delete;

	~ScopedStyleVar() {
		ImGui::PopStyleVar();
	}
};

/**
 * @struct ScopedStyleColor 
 * @brief Pushes a style color onto the stack and pops it when going out of scope.
*/
struct ScopedStyleColor {
	[[nodiscard]] inline ScopedStyleColor(ImGuiCol color, ImVec4 value) {
		ImGui::PushStyleColor(color, value);
	}

	ScopedStyleColor(ScopedStyleColor const&) = delete;
	ScopedStyleColor(ScopedStyleColor&&) = delete;

	ScopedStyleColor& operator=(ScopedStyleColor const&) = delete;
	ScopedStyleColor& operator=(ScopedStyleColor&&) = delete;

	~ScopedStyleColor() {
		ImGui::PopStyleColor();
	}
};

/**
 * @struct ScopedWidthModifier 
 * @brief Pushes an item width value onto the stack with ImGui::PushItemWidth
*/
struct ScopedWidthModifier {
	[[nodiscard]] inline ScopedWidthModifier(float value) {
		ImGui::PushItemWidth(value);
	}

	ScopedWidthModifier(ScopedWidthModifier const&) = delete;
	ScopedWidthModifier(ScopedWidthModifier&&) = delete;

	ScopedWidthModifier& operator=(ScopedWidthModifier const&) = delete;
	ScopedWidthModifier& operator=(ScopedWidthModifier&&) = delete;

	~ScopedWidthModifier() {
		ImGui::PopItemWidth();
	}
};

struct ScopedTextWrapModifier {
	[[nodiscard]] inline ScopedTextWrapModifier(float value) {
		ImGui::PushTextWrapPos(value);
	}

	ScopedTextWrapModifier(ScopedTextWrapModifier const&) = delete;
	ScopedTextWrapModifier(ScopedTextWrapModifier&&) = delete;

	ScopedTextWrapModifier& operator=(ScopedTextWrapModifier const&) = delete;
	ScopedTextWrapModifier& operator=(ScopedTextWrapModifier&&) = delete;

	~ScopedTextWrapModifier() {
		ImGui::PopTextWrapPos();
	}
};

/**
 * @brief Draw text centered horizontally inside a container.
 * @param text The text to draw. This will be an unformatted text widget.
 * @param width The width of the container the text must be centered in.
*/
void draw_text_centered(std::string const& text, float width);

} // namespace andromeda::editor::style