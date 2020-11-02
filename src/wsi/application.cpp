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
	renderer = std::make_unique<renderer::ForwardPlusRenderer>(context);
}

Application::~Application() {
	context.vulkan->device.waitIdle();
	renderer.reset(nullptr);
	window.close();
	context.world.reset(nullptr);
	context.vulkan.reset(nullptr);
}

void Application::run() {
	using namespace components;

	ecs::entity_t cam = context.world->create_entity();
	Handle<EnvMap> env_maps[2] = {
		{}, {}
	};
	env_maps[0] = context.request_env_map("data/envmaps/the_lost_city_4k.hdr");
	uint32_t envmap_idx = 0;
	// Make camera look at the center of the scene
	{
		auto& cam_data = context.world->ecs().add_component<Camera>(cam);
		cam_data.front = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
		cam_data.up = glm::normalize(glm::vec3(-1.0f, 1.0f, -1.0f));
		context.world->ecs().get_component<Transform>(cam).position = glm::vec3(4.0f, 4.0f, 4.0f);
	}

	srand(time(nullptr));

	glm::vec3 colors[]{
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1},
		{1, 0, 1},
		{0, 1, 1},
		{1, 1, 0},
		{1, 1, 1}
	};
	uint32_t color_cnt = sizeof(colors) / sizeof(glm::vec3);

	uint32_t const light_count = 50;
	for (int i = 0; i < light_count; ++i) {
		ecs::entity_t light_entity = context.world->create_entity();
		auto& light = context.world->ecs().add_component<PointLight>(light_entity);
		light.radius = rand() % 50 + 10;
		light.intensity = rand() % 100 + 20;
		light.color = colors[rand() % color_cnt];
		auto& light_trans = context.world->ecs().get_component<Transform>(light_entity);
		light_trans.position = glm::vec3(rand() % 100 - 50, rand() % 30, rand() % 40 - 20);
	}

	context.request_model("data/models/sponza-gltf-pbr/sponza.glb");

	double time = mimas_get_time();
	double last_time = time;
	while (window.is_open()) {
		window.poll_events();
		context.tasks->check_task_status();
		context.tasks->free_if_idle();

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
			renderer->resize(vk::Extent2D(size.x, size.y));
			ImGui::Image(ImGui_ImplPhobos_GetTexture(renderer->scene_image()), size);
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

			ImGui::DragFloat3("cam rotation", &cam_transform.rotation.x, 1.0f);
			if (ImGui::Button("Toggle envmap")) {
				envmap_idx++;
				envmap_idx %= 2;
			}
			cam_data.env_map = env_maps[envmap_idx];
			ImGui::Text("frame time: %f ms", delta_time * 1000.0);
			ImGui::Text("fps: %d", (int)(1.0 / delta_time));
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