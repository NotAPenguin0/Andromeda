#include <andromeda/editor/inspector.hpp>

#include <andromeda/components/hierarchy.hpp>
#include <andromeda/components/name.hpp>

#include <andromeda/editor/style.hpp>
#include <andromeda/editor/widgets/input_text.hpp>

// We'll use reflection to automatically get components to display in the UI.
#include <reflect/reflection.hpp>

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

	template<typename T>
	void operator()(meta::typed_field<C, T> field) {
		do_display(field, get_label(field).c_str(), field.get(component));
	}

private:
	template<typename T>
	std::string get_label(meta::typed_field<C, T> field) {
		return field.name() + "##field-display-" + refl.name();
	}


	template<typename T>
	void do_display(meta::typed_field<C, Handle<T>> const& meta, const char* label, Handle<T>& value) {
		ImGui::Text("%s: %d", meta.name().c_str(), value.get_id());
	}

	void do_display(meta::typed_field<C, ecs::entity_t> const& meta, const char* label, ecs::entity_t& value) {
		ImGui::Text("%s: %d", meta.name().c_str(), value);
	}

	void do_display(meta::typed_field<C, float> const& meta, const char* label, float& value) {
		ImGui::DragFloat(label, &value, 0.2f);
	}

	void do_display(meta::typed_field<C, glm::vec3> const& meta, const char* label, glm::vec3& value) {
		ImGui::DragFloat3(label, &value.x, 0.2f);
	}
	
	void do_display(meta::typed_field<C, std::string> const& meta, const char* label, std::string& value) {
		ImGui::Text("%s: %s", meta.name().c_str(), value.c_str());
	}

	void do_display(meta::typed_field<C, std::vector<ecs::entity_t>> const& meta, const char* label, std::vector<ecs::entity_t>& value) {
		ImGui::Text("%s is a vector (currently unsupported).", meta.name().c_str());
	}
};

// Note that this needs to be a special functor, since for_each_component can't accept function pointers.
template<typename C>
struct display_component {

	// Display a component of type C for a given entity.
	void operator()(thread::LockedValue<ecs::registry>& ecs, ecs::entity_t entity) {
		// First, check if this component is present on the entity. If not, we don't want to display anything.
		if (!ecs->has_component<C>(entity)) return;

		// Now obtain reflection info for the component so we can start displaying it.
		meta::reflection_info<C> refl = meta::reflect<C>();
		// Show header for the component with its name.
		std::string header = refl.name() + "##component-header";
		if (ImGui::CollapsingHeader(header.c_str())) {
			C& component = ecs->get_component<C>(entity);

			for (meta::field_variant<C> const& field : refl.fields()) {
				std::visit(display_component_field<C>{ component, refl }, field);
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