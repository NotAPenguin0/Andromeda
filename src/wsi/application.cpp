#include <andromeda/wsi/application.hpp>

#include <andromeda/components/hierarchy.hpp>
#include <andromeda/util/log.hpp>
#include <andromeda/util/version.hpp>

#include <phobos/core/vulkan_context.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/assets/importers/stb_image.h>
#include <andromeda/assets/texture.hpp>
#include <andromeda/components/static_mesh.hpp>
#include <andromeda/components/camera.hpp>
#include <andromeda/components/transform.hpp>
#include <andromeda/components/mesh_renderer.hpp>
#include <andromeda/components/point_light.hpp>

#include <mimas/mimas.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_phobos.h>
#include <imgui/imgui_impl_mimas.h>

#include <glm/glm.hpp>

namespace andromeda::wsi {

static void load_imgui_fonts(ph::VulkanContext& ctx) {
	vk::CommandBuffer command_buffer = ctx.graphics->begin_single_time(0);
	ImGui_ImplPhobos_CreateFontsTexture(command_buffer);
	ctx.graphics->end_single_time(command_buffer);

	ctx.device.waitIdle();
	ImGui_ImplPhobos_DestroyFontUploadObjects();
	ctx.graphics->free_single_time(command_buffer, 0);
}

Application::Application(size_t width, size_t height, std::string_view title)
	: window(width, height, title) {

	ph::AppSettings settings;
#if ANDROMEDA_DEBUG
	settings.enable_validation_layers = true;
#else
	settings.enabe_validation_layers = false;
#endif
	constexpr Version v = ANDROMEDA_VERSION;
	settings.version = { v.major, v.minor, v.patch };
	settings.num_threads = ftl::GetNumHardwareThreads();
	context.vulkan = std::unique_ptr<ph::VulkanContext>(ph::create_vulkan_context(*window.handle(), &io::get_console_logger(), settings));
	context.world = std::make_unique<world::World>();
	context.tasks = std::make_unique<TaskManager>();
	context.envmap_loader = std::make_unique<EnvMapLoader>(*context.vulkan);

	// Initialize imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingWithShift = false;
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	ImGui_ImplMimas_InitForVulkan(window.handle()->handle);
	ImGui_ImplPhobos_InitInfo init_info;
	init_info.context = context.vulkan.get();
	ImGui_ImplPhobos_Init(&init_info);
	io.Fonts->AddFontDefault();
	load_imgui_fonts(*context.vulkan);
	renderer = std::make_unique<renderer::Renderer>(context);
}

Application::~Application() {
	context.vulkan->device.waitIdle();
	renderer.reset(nullptr);
	window.close();
	context.world.reset(nullptr);
	context.vulkan.reset(nullptr);
}

static constexpr float quad_verts[] = {
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f
};

static constexpr uint32_t quad_indices[] = {
	0, 1, 2, 3, 4, 5
};

void Application::run() {
	using namespace components;
/*
	Handle<Texture> tex = assets::load<Texture>(context, "data/textures/dragon_texture_color.png", true);
	Handle<Texture> normal_map = assets::load<Texture>(context, "data/textures/dragon_texture_normal.png", false);
	Handle<Material> mat = assets::take<Material>({ .diffuse = tex, .normal = normal_map });
	ecs::entity_t ent = context.world->create_entity();
	auto& mesh = context.world->ecs().add_component<StaticMesh>(ent);
	auto& trans = context.world->ecs().get_component<Transform>(ent);
	auto& material = context.world->ecs().add_component<MeshRenderer>(ent);
	mesh.mesh = assets::load<Mesh>(context, "data/meshes/dragon.glb");
	trans.position = glm::vec3(0.3f, 1.3f, 0.1f);
	trans.scale = glm::vec3(0.7f, 0.7f, 0.7f);
	material.material = mat;

	ecs::entity_t floor = context.world->create_entity();
	context.world->ecs().add_component<StaticMesh>(floor).mesh =
		assets::load<Mesh>(context, "data/meshes/plane.glb");
	Handle<Texture> floor_color = assets::load<Texture>(context, "data/textures/plane_texture_color.png", true);
	Handle<Texture> floor_normal = assets::load<Texture>(context, "data/textures/plane_texture_normal.png", false);
	Handle<Material> floor_mat = assets::take<Material>({ .color = floor_color, .normal = floor_normal });
	context.world->ecs().add_component<MeshRenderer>(floor).material = floor_mat;
	auto& floor_trans = context.world->ecs().get_component<Transform>(floor);
	floor_trans.position = glm::vec3(0.0f, 0.3f, 0.0f);
	floor_trans.scale = glm::vec3(2, 1, 2);
	*/

	ecs::entity_t cam = context.world->create_entity();
	Handle<EnvMap> env_maps[2] = {
		{}, {}
	};
	uint32_t envmap_idx = 0;
	// Make camera look at the center of the scene
	{
		auto& cam_data = context.world->ecs().add_component<Camera>(cam);
		cam_data.front = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
		cam_data.up = glm::normalize(glm::vec3(-1.0f, 1.0f, -1.0f));
		context.world->ecs().get_component<Transform>(cam).position = glm::vec3(4.0f, 4.0f, 4.0f);
	}

	ecs::entity_t light_entity = context.world->create_entity();
	auto& light = context.world->ecs().add_component<PointLight>(light_entity);
	light.radius = 7.9f;
	light.intensity = 15.8f;
	auto& light_trans = context.world->ecs().get_component<Transform>(light_entity);
	light_trans.position = glm::vec3(-0.3f, 0.4f, 1.0f);
	env_maps[1] = context.request_env_map("data/envmaps/moonless_golf_4k.hdr");
	auto initialize = [&env_maps, this] {
		env_maps[0] = context.request_env_map("data/envmaps/birchwood_4k.hdr");
//		env_maps[1] = context.request_env_map("data/envmaps/moonless_golf_4k.hdr");
		Handle<Mesh> sphere = context.request_mesh("data/meshes/sphere.glb");

		Handle<Texture> color = context.request_texture("data/textures/gold-scuffed_basecolor.png", true);
		Handle<Texture> normal = context.request_texture("data/textures/gold-scuffed_normal.png", false);
		Handle<Texture> metallic = context.request_texture("data/textures/gold-scuffed_metallic.png", false);
		Handle<Texture> roughness = context.request_texture("data/textures/gold-scuffed_roughness.png", false);
		Handle<Texture> ao = context.request_texture("data/textures/blank.png", false);


		Handle<Material> pbr_mat = assets::take<Material>(Material{
				.color = color,
				.normal = normal,
				.metallic = metallic,
				.roughness = roughness,
				.ambient_occlusion = ao
			}
		);

		Handle<Texture> color2 = context.request_texture("data/textures/iced-over-ground7-albedo.png", true);
		Handle<Texture> normal2 = context.request_texture("data/textures/iced-over-ground7-Normal-dx.png", false);
		Handle<Texture> metallic2 = context.request_texture("data/textures/iced-over-ground7-Metallic.png", false);
		Handle<Texture> roughness2 = context.request_texture("data/textures/iced-over-ground7-Roughness.png", false);
		Handle<Texture> ao2 = context.request_texture("data/textures/iced-over-ground7-ao.png", false);


		Handle<Material> pbr_mat2 = assets::take<Material>(Material{
			.color = color2,
			.normal = normal2,
			.metallic = metallic2,
			.roughness = roughness2,
			.ambient_occlusion = ao2
			}
		);

		Handle<Texture> color3 = context.request_texture("data/textures/grimy-metal-albedo.png", true);
		Handle<Texture> normal3 = context.request_texture("data/textures/grimy-metal-normal-dx.png", false);
		Handle<Texture> metallic3 = context.request_texture("data/textures/grimy-metal-metallic.png", false);
		Handle<Texture> roughness3 = context.request_texture("data/textures/grimy-metal-roughness.png", false);
		Handle<Texture> ao3 = context.request_texture("data/textures/blank.png", false);

		Handle<Material> pbr_mat3 = assets::take<Material>(Material{
			.color = color3,
			.normal = normal3,
			.metallic = metallic3,
			.roughness = roughness3,
			.ambient_occlusion = ao3
			}
		);

		Handle<Material> materials[]{ pbr_mat, pbr_mat2, pbr_mat3 };

		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				ecs::entity_t entt = context.world->create_entity();
				auto& trans = context.world->ecs().get_component<Transform>(entt);
				trans.position.x = -3.0f + j * 1.5f;
				trans.position.y = -3.0f + i * 1.5f;
				trans.position.z = -1.5f;
				auto& mesh = context.world->ecs().add_component<StaticMesh>(entt);
				mesh.mesh = sphere;
				auto& rend = context.world->ecs().add_component<MeshRenderer>(entt);
				constexpr size_t amt = sizeof(materials) / sizeof(Handle<Material>);
				rend.material = materials[(i + j) % amt];
			}
		}
	};
//	initialize();

	double time = mimas_get_time();
	double last_time = time;
	while (window.is_open()) {
		window.poll_events();

		ImGui_ImplMimas_NewFrame();
		ImGui::NewFrame();
		
		time = mimas_get_time();
		double delta_time = time - last_time;
		last_time = time;

		// Create ImGui dockspace
		ImGuiWindowFlags flags =
			ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Dockspace", nullptr, flags);
		ImGui::PopStyleVar(3);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
				ImGuiDockNodeFlags_None);
		}

		if (ImGui::Begin("Scene view", nullptr)) {
			auto size = ImGui::GetContentRegionAvail();
//			renderer->resize_attachments(size.x, size.y);
			ImGui::Image(ImGui_ImplPhobos_GetTexture(renderer->scene_image()), { 1280, 720 });
		}
		ImGui::End();
		auto& cam_transform = context.world->ecs().get_component<Transform>(cam);
		auto& cam_data = context.world->ecs().get_component<Camera>(cam);
		if (ImGui::Begin("Debug edit", nullptr)) {
			auto display_transform = [&](const char* name, ecs::entity_t e) {
				if (ImGui::CollapsingHeader(name)) {
					auto& trans = context.world->ecs().get_component<Transform>(e);
					ImGui::DragFloat3("position", &trans.position.x, 0.1f);
					ImGui::DragFloat3("rotation", &trans.rotation.x, 1.0f);
					ImGui::DragFloat3("scale", &trans.scale.x, 0.1f);
				}
			};

			auto display_light = [&](const char* name, ecs::entity_t e) {
				if (ImGui::CollapsingHeader(name)) {
					auto& trans = context.world->ecs().get_component<Transform>(e);
					ImGui::DragFloat3("position", &trans.position.x, 0.1f);
					ImGui::DragFloat3("rotation", &trans.rotation.x, 1.0f);
					ImGui::DragFloat3("scale", &trans.scale.x, 0.1f);
					auto& light = context.world->ecs().get_component<PointLight>(e);
					ImGui::ColorEdit3("color", &light.color.r);
					ImGui::DragFloat("radius", &light.radius, 0.1f);
					ImGui::DragFloat("intensity", &light.intensity, 0.1f);
				}
			};

//			display_transform("dragon", ent);
//			display_transform("floor", floor);
			display_light("light", light_entity);

			ImGui::Checkbox("Enable ambient lighting", &renderer->get_lighting_pass().enable_ambient);
			ImGui::DragFloat3("cam rotation", &cam_transform.rotation.x, 1.0f);
			if (ImGui::Button("Toggle envmap")) {
				envmap_idx++;
				envmap_idx %= 2;
			}
			cam_data.env_map = env_maps[envmap_idx];
			ImGui::Text("frame time: %f ms", delta_time * 1000.0);
			ImGui::Text("fps: %d", (int)(1.0 / delta_time));
			if (ImGui::Button("Initialize")) {
				initialize();
			}
		}
		ImGui::End();

		constexpr float max_pitch = 89.0f;
		constexpr float min_pitch = -89.0f;
		if (cam_transform.rotation.x > max_pitch) cam_transform.rotation.x = max_pitch;
		if (cam_transform.rotation.x < min_pitch) cam_transform.rotation.x = min_pitch;
		float const cos_pitch = std::cos(glm::radians(cam_transform.rotation.x));
		float const cos_yaw = std::cos(glm::radians(cam_transform.rotation.y));
		float const sin_pitch = std::sin(glm::radians(cam_transform.rotation.x));
		float const sin_yaw = std::sin(glm::radians(cam_transform.rotation.y));
		cam_data.front.x = cos_pitch * cos_yaw;
		cam_data.front.y = sin_pitch;
		cam_data.front.z = cos_pitch * sin_yaw;
		cam_data.front = glm::normalize(cam_data.front);
		glm::vec3 right = glm::normalize(glm::cross(cam_data.front, glm::vec3(0, 1, 0)));
		cam_data.up = glm::normalize(glm::cross(right, cam_data.front));

		// End ImGui dockspace
		ImGui::End();

		renderer->render(context);
	}
}

}