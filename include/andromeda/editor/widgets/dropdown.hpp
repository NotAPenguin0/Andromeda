#pragma once

#include <andromeda/graphics/imgui.hpp>

#include <concepts>
#include <functional>
#include <iterator>
#include <optional>
#include <string>


namespace andromeda::editor {

/**
 * @class DropDown 
 * @brief Represents a dropdown selectable widget.
 * @tparam T Type of each element to display in the dropdown widget.
 * @tparam It Type of the iterator pointing to the elements to display.
*/
template<typename T, typename It>
class DropDown {
public:

	/**
	 * @brief Creates a dropdown menu.
	 * @param name Label given to the dropdown.
	 * @param preview Label previewed before opening the dropdown.
	 * @param begin First element to display.
	 * @param end Last element to display.
	 * @param display Function used to display each element. This function must return true
	 *		  if the element is selected.
	*/
	DropDown(std::string const& name, std::string const& preview, It begin, It end, std::function<bool(T const&)> display)
		requires std::same_as<typename It::value_type, T> : selected(end), end_it(end) {
		if (ImGui::BeginCombo(name.c_str(), preview.c_str())) {
			// Loop over each item in the list of items and display them one by one, updating the 
			// selected pointer as we go
			for (It cur = begin; cur != end; ++cur) {
				if (display(*cur)) {
					selected = cur;
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}
	}

	/**
	 * @brief Get the currently selected element
	 * @return The selected element, or std::nullopt if there is none.
	*/
	std::optional<T> get_selected() {
		if (selected == end_it) return std::nullopt;
		return *selected;
	}

	/**
	 * @brief Get the currently selected element
	 * @return The selected element, or std::nullopt if there is none.
	*/
	std::optional<T const> get_selected() const {
		if (selected == end_it) return std::nullopt;
		return *selected;
	}

private:
	It selected;
	It end_it; // end iterator stored so we have an invalid value.
};

template<typename F, typename It>
DropDown(std::string const&, std::string const&, It, It, F)->DropDown<typename It::value_type, It>;

} // namespace andromeda::editor