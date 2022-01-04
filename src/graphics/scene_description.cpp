#include <andromeda/graphics/scene_description.hpp>
#include <andromeda/graphics/material.hpp>
#include <andromeda/graphics/texture.hpp>
#include <andromeda/assets/assets.hpp>

#include <andromeda/components/environment.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace andromeda::gfx {

void SceneDescription::add_draw(Handle<gfx::Mesh> mesh, Handle<gfx::Material> material, glm::mat4 const& transform) {
	draws.push_back(Draw{mesh, material});
    draw_transforms.push_back(transform);
}

void SceneDescription::add_material(Handle<gfx::Material> material) {
	// Check if valid handle
	if (material == Handle<gfx::Material>::none) return;
	// Check is_ready() before calling get()
	if (!assets::is_ready(material)) return;

	gfx::Material* mat = assets::get(material);
	// Add each material texture individually
	add_texture(mat->albedo);
    add_texture(mat->normal);
    add_texture(mat->metal_rough);
    add_texture(mat->occlusion);
}

void SceneDescription::add_light(PointLight const& light, glm::vec3 const& position) {
    gpu::PointLight info {};
    info.pos_radius = glm::vec4(position, light.radius);
    info.color_intensity = glm::vec4(light.color, light.intensity);
    point_lights.push_back(info);
}

void SceneDescription::add_viewport(gfx::Viewport const& vp, thread::LockedValue<const ecs::registry> const& ecs, ecs::entity_t camera) {
    auto const& cam = ecs->get_component<Camera>(camera);
    auto const& transform = ecs->get_component<Transform>(camera);
	CameraInfo& info = cameras[vp.index()];
    info.active = true;

    // If an environment component is present, set the handle accordingly
    if (ecs->has_component<::andromeda::Environment>(camera)) {
        info.environment = ecs->get_component<::andromeda::Environment>(camera).environment;
    }
    // Once atmospheric scattering is added, the atmosphere data can be retrieved here

    // Postprocessing settings
    if (ecs->has_component<PostProcessingSettings>(camera)) {
        auto const& settings = ecs->get_component<PostProcessingSettings>(camera);
        info.min_log_luminance = settings.min_log_luminance;
        info.max_log_luminance = settings.max_log_luminance;
    }

	// Projection matrix

	float aspect = (float)vp.width() / (float)vp.height();
    info.projection = glm::perspective(glm::radians(cam.fov), aspect, cam.near, cam.far);
	// Vulkan needs upside down projection matrix
    info.projection[1][1] *= -1;

	// View matrix

	float const cos_pitch = std::cos(glm::radians(transform.rotation.x));
	float const cos_yaw = std::cos(glm::radians(transform.rotation.y));
	float const sin_pitch = std::sin(glm::radians(transform.rotation.x));
	float const sin_yaw = std::sin(glm::radians(transform.rotation.y));

	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;

	front.x = cos_pitch * cos_yaw;
	front.y = sin_pitch;
	front.z = cos_pitch * sin_yaw;
	front = glm::normalize(front);
	right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
	up = glm::normalize(glm::cross(right, front));
    info.view = glm::lookAt(transform.position, transform.position + front, up);

	// Precompute projection-view matrix as its commonly used.
    info.proj_view = info.projection * info.view;

    // Other values
    info.position = transform.position;
}

void SceneDescription::set_default_albedo(Handle<gfx::Texture> handle) {
    textures.default_albedo = handle;
}

void SceneDescription::set_default_normal(Handle<gfx::Texture> handle) {
    textures.default_normal = handle;
}

void SceneDescription::set_default_metal_rough(Handle<gfx::Texture> handle) {
    textures.default_metal_rough = handle;
}

void SceneDescription::set_default_occlusion(Handle<gfx::Texture> handle) {
    textures.default_occlusion = handle;
}

void SceneDescription::set_default_environment(Handle<gfx::Environment> handle) {
    default_env = handle;
}

void SceneDescription::set_brdf_lut(Handle<gfx::Texture> handle) {
    textures.brdf_lut = handle;
}

void SceneDescription::reset() {
	draws.clear();
	textures.views.clear();
	textures.id_to_index.clear();
    point_lights.clear();

	for (auto& cam : cameras) {
		cam.active = false;
        cam.environment = Handle<gfx::Environment>::none;
        cam.min_log_luminance = 0.0;
        cam.max_log_luminance = 0.0;
	}

    // Push default textures to the texture map
    add_texture(textures.default_albedo);
    add_texture(textures.default_normal);
    add_texture(textures.default_metal_rough);
    add_texture(textures.default_occlusion);
}

SceneDescription::MaterialTextures SceneDescription::get_material_textures(Handle<gfx::Material> material) const {
	if (material == Handle<gfx::Material>::none) return {};
	if (!assets::is_ready(material)) return {};

	gfx::Material* mat = assets::get(material);

	return MaterialTextures{
		.albedo = textures.id_to_index[mat->albedo ? mat->albedo : textures.default_albedo],
        .normal = textures.id_to_index[mat->normal ? mat->normal : textures.default_normal],
        .metal_rough = textures.id_to_index[mat->metal_rough ? mat->metal_rough : textures.default_metal_rough],
        .occlusion = textures.id_to_index[mat->occlusion ? mat->occlusion : textures.default_occlusion]
	};
}

void SceneDescription::add_texture(Handle<gfx::Texture> texture) {
	// Don't add empty textures, they will be given a default value instead
	if (texture == Handle<gfx::Texture>::none) return;
	// Don't add textures that are still loading
	if (!assets::is_ready(texture)) return;

	gfx::Texture* tex = assets::get(texture);

	// Add it to the list and set the index mapping
	textures.views.push_back(tex->view);
	textures.id_to_index[texture] = textures.views.size() - 1;
}


}