#pragma once

#include <andromeda/graphics/imgui.hpp>

#include <string>

namespace andromeda::editor {

/**
 * @class Table 
 * @brief Wrapper class around ImGui::BeginTable() and ImGui::EndTable()
*/
class Table {
public:
	struct Flags {
		
	};

	struct ColumnFlags {
		/**
		 * @brief This flag indicates that table columns cannot be moved, sorted, hidden or resized by the
		 *		  user.
		*/
		static constexpr ImGuiTableColumnFlags locked =
			ImGuiTableColumnFlags_NoSort
			| ImGuiTableColumnFlags_NoReorder
			| ImGuiTableColumnFlags_NoHide
			| ImGuiTableColumnFlags_NoDirectResize_;
	};

	/**
	 * @brief Create a table with given parameters. This will call ImGui::BeginTable() and start the table context.
	 * @param name String identifier for the table. ImGui uses this to calculate a unique ID.
	 * @param column_count The amount of columns the table has.
	 * @param flags Table flags for styling.
	 * @param size Size of the table. Leave a component at zero to automatically determine the size
	 *			   based on the context.
	*/
	Table(std::string const& name, uint32_t column_count, ImGuiTableFlags flags = {}, ImVec2 size = ImVec2{ 0, 0 });

	~Table();

	/**
	 * @brief Whether the table is visible on the screen. If this is false you can omit providing content.
	 * @return The result of ImGui::BeginTable().
	*/
	bool visible() const;

	/**
	 * @brief Declares a column.
	 * @param name String identifier of the column. If headers are enabled this will be displayed as a label.
	 * @param flags Column flags. This may not include ImGuiTableColumnFlags_WidthFixed, for this use the 
	 *				provided overload.
	*/
	void column(std::string const& name, ImGuiTableColumnFlags flags);

	/**
	 * @brief Declares a column with a fixed width.
	 * @param name String identifier of the column. If headers are enabled this will be displayed as a label.
	 * @param flags Column flags. ImGuiTableColumnFlags_WidthFixed is provided automatically.
	 * @param width Fixed width of the column.
	*/
	void column(std::string const& name, ImGuiTableColumnFlags flags, float width);

	/**
	 * @brief Display a row with column headers. Must be called after all columns have been
	 *		  declared using column().
	 * @param pin Whether to pin this row. Setting this to false will cause the header to disappear when scrolling.
	*/
	void header_row(bool pin = true);

	/**
	 * @brief When drawing, use this to move to the next column.
	 *		  This will also automatically move to the next row.
	*/
	void next_column();

private:
	bool begin_result = false;
};

} // namespace andromeda::editor