#include <andromeda/editor/widgets/table.hpp>

namespace andromeda::editor {

Table::Table(std::string const& name, uint32_t column_count, ImGuiTableFlags flags, ImVec2 size) {
	begin_result = ImGui::BeginTable(name.c_str(), column_count, flags, size);
}

Table::~Table() {
	if (begin_result) {
		ImGui::EndTable();
	}
}

bool Table::visible() const {
	return begin_result;
}

void Table::column(std::string const& name, ImGuiTableColumnFlags flags) {
	if (flags & ImGuiTableColumnFlags_WidthFixed) {
		LOG_WRITE(LogLevel::Warning, "Invalid table flags supplied to table column.");
	}

	ImGui::TableSetupColumn(name.c_str(), flags);
}

void Table::column(std::string const& name, ImGuiTableColumnFlags flags, float width) {
	ImGui::TableSetupColumn(name.c_str(), flags | ImGuiTableColumnFlags_WidthFixed, width);
}

void Table::header_row(bool pin) {
	if (pin) {
		ImGui::TableSetupScrollFreeze(0, 1);
	}

	ImGui::TableHeadersRow();
}

void Table::next_column() {
	ImGui::TableNextColumn();
}

} // namespace andromeda::editor