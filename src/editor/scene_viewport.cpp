#include <andromeda/editor/scene_viewport.hpp>

#include <andromeda/editor/widgets/dropdown.hpp>
#include <andromeda/editor/widgets/floating_panel.hpp>
#include <andromeda/editor/widgets/image.hpp>
#include <andromeda/editor/widgets/menu_bar.hpp>

#include <andromeda/editor/style.hpp>

#include <andromeda/components/camera.hpp>
#include <andromeda/components/hierarchy.hpp>
#include <andromeda/components/name.hpp>

#include <string>

using namespace std::literals::string_literals;


namespace andromeda::editor {

// Styling options

// Offset in the window of the options panel, starting from the top-left corner
static ImVec2 const panel_offset = ImVec2(15, 45);
// Size of the options panel.
static ImVec2 const panel_size = ImVec2(150, 40);
// Opacity of the options panel.
static float const panel_alpha = 0.85;

// Returns a string to display a camera entity. Right now this identifies the camera by entity ID, but we can use
// the entity name later.
static std::string camera_string_display(thread::LockedValue<ecs::registry const>& ecs, 
	ecs::entity_t entity, gfx::Viewport const& viewport) {
	if (entity == ecs::no_entity) {
		return gfx::Viewport::local_string(viewport, ICON_FA_CAMERA " No camera##");
	}
	return gfx::Viewport::local_string(viewport, ICON_FA_CAMERA " " + ecs->get_component<Name>(entity).name + "##");
}

SceneViewport::SceneViewport(gfx::Viewport viewport, gfx::Context& ctx, gfx::Renderer& renderer, World const& world) {
	// When this class is created this signals that this viewport must be shown, so we reset our viewport-static 'is_open' value to true.
	PerViewportStatic& vp_data = per_viewport[viewport.index()];
	vp_data.is_open = true;

	show_viewport(viewport, ctx, renderer, world);
	show_debug_views(viewport, ctx, renderer);
}


bool SceneViewport::is_visible() const {
	return visible;
}

void SceneViewport::show_viewport(gfx::Viewport viewport, gfx::Context& ctx, gfx::Renderer& renderer, World const& world) {
	// We want to fit the entire image inside the viewport without any padding.
	// ScopedStyleVar will automatically call PopStyleVar on destruction.
	auto padding = style::ScopedStyleVar<ImVec2>(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));

	// Set visible variable and display the UI if it's visible.
	ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
	PerViewportStatic& vp_data = per_viewport[viewport.index()];
	// Make sure to set initial size. This is to make the program not crash trying to 
	// resize an attachment to zero size.
	ImGui::SetNextWindowSize({ 600, 800 }, ImGuiCond_FirstUseEver);
	if (visible = ImGui::Begin((std::string("Viewport ") + std::to_string(viewport.index())).c_str(), &vp_data.is_open, flags); visible) {
		show_menu_bar(viewport, renderer);

		// Resize the viewport to fit the window. This way the user can easily resize attachments from the UI.
		ImVec2 vp_size = ImGui::GetContentRegionAvail();
		renderer.resize_viewport(ctx, viewport, vp_size.x, vp_size.y);

		display_image(ctx.get_attachment(viewport.target()).view);

		show_panel(viewport, renderer, world);
	}
	ImGui::End();

	// If the viewport was closed this frame, delete it.
	if (!vp_data.is_open) {
		renderer.destroy_viewport(viewport);
	}
}

void SceneViewport::show_menu_bar(gfx::Viewport viewport, gfx::Renderer& renderer) {
	PerViewportStatic& vp_data = per_viewport[viewport.index()];
	auto padding = style::ScopedStyleVar<ImVec2>(ImGuiStyleVar_WindowPadding, style::default_style().WindowPadding);
	MenuBar menu{};
	{
		MenuBar::MenuBuilder options = menu.submenu(
			gfx::Viewport::local_string(viewport, "Options##viewport"));
		{
			MenuBar::MenuBuilder show_views_menu = options.submenu(
				gfx::Viewport::local_string(viewport, "Show debug view##viewport"));
			{

				for (auto const& view : renderer.get_debug_views(viewport)) {
					bool& open = vp_data.debug_views[view];
					show_views_menu.item(view, open);
				}
			}
		}
	}
}

void SceneViewport::show_panel(gfx::Viewport viewport, gfx::Renderer& renderer, World const& world) {
	FloatingPanel panel{ gfx::Viewport::local_string(viewport, "##viewport_options"),
				panel_offset, panel_size, panel_alpha };
	panel.display([&viewport, &world, &renderer]() {

		auto width = style::ScopedWidthModifier(ImGui::GetContentRegionAvailWidth());

		// Gain thread-safe access to the ECS
		thread::LockedValue<ecs::registry const> ecs = world.ecs();

		// Get current camera so we can display a preview
		ecs::entity_t camera = viewport.camera();
		// Get list of valid camera entities to select from.
		ecs::const_component_view<Hierarchy, Camera> camera_list = ecs->view<Hierarchy, Camera>();

		DropDown camera_selector = DropDown(
			gfx::Viewport::local_string(viewport, "##camselect"), // label
			camera_string_display(ecs, camera, viewport), // preview string
			camera_list.begin(), // range of elements to display
			camera_list.end(),
			// display function
			[&viewport, &ecs](std::tuple<Hierarchy const&, Camera const&> const& camera) -> bool {
				// Get entity ID of camera.
				ecs::entity_t entity = std::get<Hierarchy const&>(camera).this_entity;

				std::string str = camera_string_display(ecs, entity, viewport);
				bool sel = false;
				ImGui::Selectable(str.data(), &sel);
				return sel;
			}
		);

		// Get selected value and update camera
		if (auto selected = camera_selector.get_selected(); selected != std::nullopt) {
			ecs::entity_t entity = std::get<Hierarchy const&>(*selected).this_entity;
			renderer.set_viewport_camera(viewport, entity);
		}
	});
}

void SceneViewport::show_debug_views(gfx::Viewport viewport, gfx::Context& ctx, gfx::Renderer& renderer) {
	PerViewportStatic& vp_data = per_viewport[viewport.index()];
	auto padding = style::ScopedStyleVar<ImVec2>(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
	auto views = renderer.get_debug_views(viewport);
	for (auto const& view : views) {
		ph::ImageView image = ctx.get_attachment(view).view;
		bool& open = vp_data.debug_views[view];
		if (open) {
			size_next_window_to_image(image);
			ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
			if (ImGui::Begin(gfx::Viewport::local_string(viewport, view + "##window").c_str(), &open, flags)) {
				display_image(image);
			}
			ImGui::End();
		}
	}
}

} // namespace andromeda::editor