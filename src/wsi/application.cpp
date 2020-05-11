#include <andromeda/wsi/application.hpp>

#include <andromeda/components/hierarchy.hpp>
#include <andromeda/util/log.hpp>
#include <andromeda/util/version.hpp>

#include <phobos/core/vulkan_context.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/assets/texture.hpp>
#include <andromeda/components/static_mesh.hpp>
#include <andromeda/components/camera.hpp>
#include <andromeda/components/transform.hpp>
#include <andromeda/components/mesh_renderer.hpp>
#include <andromeda/components/point_light.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_phobos.h>
#include <imgui/imgui_impl_mimas.h>

namespace andromeda::wsi {

static void load_imgui_fonts(ph::VulkanContext& ctx, vk::CommandPool command_pool) {
	vk::CommandBufferAllocateInfo buf_info;
	buf_info.commandBufferCount = 1;
	buf_info.commandPool = command_pool;
	buf_info.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer command_buffer = ctx.device.allocateCommandBuffers(buf_info)[0];

	vk::CommandBufferBeginInfo begin_info = {};
	begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	command_buffer.begin(begin_info);

	ImGui_ImplPhobos_CreateFontsTexture(command_buffer);

	vk::SubmitInfo end_info = {};
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers = &command_buffer;
	command_buffer.end();

	ctx.graphics_queue.submit(end_info, nullptr);

	ctx.device.waitIdle();
	ImGui_ImplPhobos_DestroyFontUploadObjects();
	ctx.device.freeCommandBuffers(command_pool, command_buffer);
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
	context.vulkan = std::unique_ptr<ph::VulkanContext>(ph::create_vulkan_context(*window.handle(), &io::get_console_logger(), settings));
	context.world = std::make_unique<world::World>();

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
	vk::CommandPoolCreateInfo command_pool_info;
	command_pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	vk::CommandPool command_pool = context.vulkan->device.createCommandPool(command_pool_info);
	load_imgui_fonts(*context.vulkan, command_pool);
	context.vulkan->device.destroyCommandPool(command_pool);

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
	// Make camera look at the center of the scene
	auto& cam_data = context.world->ecs().add_component<Camera>(cam);
	cam_data.front = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
	cam_data.up = glm::normalize(glm::vec3(-1.0f, 1.0f, -1.0f));
	context.world->ecs().get_component<Transform>(cam).position = glm::vec3(4.0f, 4.0f, 4.0f);

	ecs::entity_t light_entity = context.world->create_entity();
	auto& light = context.world->ecs().add_component<PointLight>(light_entity);
	light.radius = 7.9f;
	light.intensity = 15.8f;
	auto& light_trans = context.world->ecs().get_component<Transform>(light_entity);
	light_trans.position = glm::vec3(-0.3f, 0.4f, 1.0f);

	Handle<Mesh> sphere = assets::load<Mesh>(context, "data/meshes/sphere.glb");

	Handle<Texture> color = assets::load<Texture>(context, "data/textures/rustediron2_basecolor.png", true);
	Handle<Texture> normal = assets::load<Texture>(context, "data/textures/rustediron2_normal.png", false);
	Handle<Texture> metallic = assets::load<Texture>(context, "data/textures/rustediron2_metallic.png", false);
	Handle<Texture> roughness = assets::load<Texture>(context, "data/textures/rustediron2_roughness.png", false);
	Handle<Texture> ao = assets::load<Texture>(context, "data/textures/blank.png", false);

	Handle<Material> pbr_mat = assets::take<Material>(Material{
			.color = color,
			.normal = normal,
			.metallic = metallic,
			.roughness = roughness,
			.ambient_occlusion = ao
		}
	);

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
			rend.material = pbr_mat;
		}
	}

	while (window.is_open()) {
		window.poll_events();

		ImGui_ImplMimas_NewFrame();
		ImGui::NewFrame();
		
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
		}
		ImGui::End();

		// End ImGui dockspace
		ImGui::End();

		renderer->render(context);
	}
}

}