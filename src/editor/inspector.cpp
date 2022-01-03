#include <andromeda/editor/inspector.hpp>

#include <andromeda/components/hierarchy.hpp>
#include <andromeda/components/name.hpp>

#include <andromeda/editor/style.hpp>
#include <andromeda/editor/widgets/input_text.hpp>
#include <andromeda/editor/widgets/tooltip.hpp>

// We'll use reflection to automatically get components to display in the UI.
#include <reflect/reflection.hpp>

#include <string>
using namespace std::literals::string_literals;

namespace andromeda::editor {

void Inspector::display(World& world) {
	if (visible) {
		if (ImGui::Begin("Inspector##widget", &visible)) {

			float width = ImGui::GetContentRegionAvailWidth();
			if (selected_entity != ecs::no_entity) width /= 2.0f;
			if (ImGui::BeginChild("##inspector-tree", ImVec2(width, 0), false)) {
				show_entity_list(world);
			}
			ImGui::EndChild();
			// Only display details panel if there is a selected entity.
			if (selected_entity != ecs::no_entity) {
				ImGui::SameLine();
				
				if (ImGui::BeginChild("##inspector-details-panel", ImVec2(0, 0), true)) {
					show_details_panel(world);
				}
				ImGui::EndChild();
			}
		}
		ImGui::End();
	}
}

bool& Inspector::is_visible() {
	return visible;
}

ecs::entity_t& Inspector::selected() {
	return selected_entity;
}

ecs::entity_t const& Inspector::selected() const {
	return selected_entity;
}

void Inspector::show_entity_list(World& world) {
	// The algorithm for displaying entities in a tree-like structure will work as follows:
	// Loop over all entities and filter out those that have the root entity as parent.
	// For each of those entities, create a TreeNode. Then we loop over all children and recursively 
	// display those. 
	// When displaying a node, we will use TreeNodeEx() and add the ImGuiTreeNodeFlags_Leaf flag for 
	// entities with no children.

	thread::LockedValue<ecs::registry> ecs = world.ecs();
	for (auto [hierarchy] : ecs->view<Hierarchy>()) {
		// This entity is a root entity.
		if (hierarchy.parent == world.root()) {
			show_entity_tree_item(ecs, hierarchy.this_entity);
		}
	}
}

void Inspector::show_entity_tree_item(thread::LockedValue<ecs::registry>& ecs, ecs::entity_t entity) {
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen
		| ImGuiTreeNodeFlags_OpenOnArrow;
	Hierarchy& hierarchy = ecs->get_component<Hierarchy>(entity);
	// Add leaf flag if there are no children, as these nodes can't be expanded.
	if (hierarchy.children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
	// Add selected flag if this entity is the selected entity
	if (entity == selected_entity) flags |= ImGuiTreeNodeFlags_Selected;

	ImVec4 color = ImVec4(174 / 255.0f, 174 / 255.0f, 174 / 255.0f, 0.31f);
	auto cvar = style::ScopedStyleColor(ImGuiCol_Header, color);

	std::string label = ecs->get_component<Name>(entity).name + "##treeitem";
	if (ImGui::TreeNodeEx(label.c_str(), flags)) {
		// When clicking an entity it becomes the selected entity.
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			selected_entity = entity;
		}

		for (ecs::entity_t child : hierarchy.children) {
			show_entity_tree_item(ecs, child);
		}
		ImGui::TreePop();
	}
}

namespace impl {

// Visitor class for each field type.
template<typename C>
struct display_component_field {
	// Reference to the component we are displaying.
	C& component;
	meta::reflection_info<C> const& refl;

	void operator()(meta::field<C> field) {
		
		// Only actually display if the editor_hide flag was not set
		if (field.flags() & meta::field_flags::editor_hide) return;

		// Dispatch to the proper field type overload, and pass parameters.
		meta::dispatch(field, component, [this, &field](auto& value) {
			do_display(value, field, get_label(field).c_str());
		});

		// After displaying the field, we add a tooltip if one is present
		if (!field.tooltip().empty()) {
			show_tooltip(field.tooltip());
		}
	}

private:
	std::string get_label(meta::field<C> field) const {
		return field.name() + "##field-display-" + refl.name();
	}

	template<typename T>
	void do_display(Handle<T>& value, meta::field<C> const& meta, const char* label) {
		ImGui::Text("%s: %ld", meta.name().c_str(), value.get_id());
	}

	void do_display(ecs::entity_t& value, meta::field<C> const& meta, const char* label) {
		ImGui::Text("%s: %ld", meta.name().c_str(), value);
	}

    void do_display(bool& value, meta::field<C> const& meta, const char* label) {
        ImGui::Checkbox(label, &value);

    }

	void do_display(float& value, meta::field<C> const& meta, const char* label) {
		if (meta.flags() & meta::field_flags::no_limits) {
			ImGui::DragFloat(label, &value, meta.drag_speed());
		}
		else {
			if (meta.max() > meta.min()) {
				ImGui::DragFloat(label, &value, meta.drag_speed(), meta.min(), meta.max());
			}
			else {
				ImGui::DragFloat(label, &value, meta.drag_speed(), meta.min());
			}
		}
	}

	void do_display(glm::vec3& value, meta::field<C> const& meta, const char* label) {
		if (meta.flags() & meta::field_flags::no_limits) {
			ImGui::DragFloat3(label, &value.x, meta.drag_speed());
		}
		else {
			if (meta.max() > meta.min()) {
				ImGui::DragFloat(label, &value.x, meta.drag_speed(), meta.min(), meta.max());
			}
			else {
				ImGui::DragFloat(label, &value.x, meta.drag_speed(), meta.min());
			}
		}
	}
	
	void do_display(std::string& value, meta::field<C> const& meta, const char* label) {
		ImGui::Text("%s: %s", meta.name().c_str(), value.c_str());
	}

	void do_display(std::vector<ecs::entity_t>& value, meta::field<C> const& meta, const char* label) {
		ImGui::Text("%s is a vector (currently unsupported).", meta.name().c_str());
	}
};

// Note that this needs to be a special functor, since for_each_component can't accept function pointers.
template<typename C>
struct display_component {
	constexpr const char* get_component_icon() const {
		if constexpr (std::is_same_v<C, Transform>) return ICON_FA_ARROWS_ALT;
		if constexpr (std::is_same_v<C, MeshRenderer>) return ICON_FA_CUBES;
		if constexpr (std::is_same_v<C, Camera>) return ICON_FA_CAMERA;
        if constexpr (std::is_same_v<C, PointLight>) return ICON_FA_LIGHTBULB;
        if constexpr (std::is_same_v<C, Environment>) return ICON_FA_GLOBE_EUROPE;
        if constexpr (std::is_same_v<C, PostProcessingSettings>) return ICON_FA_IMAGE;
        if constexpr (std::is_same_v<C, DirectionalLight>) return ICON_FA_SUN;
		return "";
	}

	// Display a component of type C for a given entity.
	void operator()(thread::LockedValue<ecs::registry>& ecs, ecs::entity_t entity) {
		// First, check if this component is present on the entity. If not, we don't want to display anything.
		if (!ecs->has_component<C>(entity)) return;

		// Now obtain reflection info for the component so we can start displaying it.
		meta::reflection_info<C> const& refl = meta::reflect<C>();

		// Get flags for this component, maybe we need to hide it.
		plib::bit_flag<meta::type_flags> flags = refl.flags();

		// Do not display components flagged with editor::hide.
		if (flags & meta::type_flags::editor_hide) return;

		// Show header for the component with its name.
		std::string header = get_component_icon() + " "s + refl.name() + "##component-header";
		if (ImGui::CollapsingHeader(header.c_str())) {
			C& component = ecs->get_component<C>(entity);
			display_component_field<C> display_func{ component, refl };
			for (meta::field<C> const& field : refl.fields()) {
				display_func(field);
			}
		}
	}
};

}

void Inspector::show_details_panel(World& world) {
	// This function can assume selected_entity is not ecs::no_entity.

	thread::LockedValue<ecs::registry> ecs = world.ecs();
	// We'll call display_component for each existing component.
	meta::for_each_component<impl::display_component>(ecs, selected_entity);
}

}