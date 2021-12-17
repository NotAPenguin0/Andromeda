#include <andromeda/graphics/scene_description.hpp>
#include <andromeda/graphics/material.hpp>
#include <andromeda/graphics/texture.hpp>
#include <andromeda/assets/assets.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace andromeda::gfx {

void SceneDescription::add_draw(Handle<gfx::Mesh> mesh, Handle<gfx::Material> material, glm::mat4 const& transform) {
	draws.push_back(Draw{mesh, material, transform});
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
    add_texture(mat->metallic);
    add_texture(mat->roughness);
    add_texture(mat->occlusion);
}

void SceneDescription::add_light(PointLight const& light, glm::vec3 const& position) {
    gpu::PointLight info {};
    info.pos_radius = glm::vec4(position, light.radius);
    info.color_intensity = glm::vec4(light.color, light.intensity);
    point_lights.push_back(info);
}

void SceneDescription::add_viewport(gfx::Viewport const& vp, Transform const& cam_transform, Camera const& cam) {
	CameraMatrices& camera = cameras[vp.index()];
	camera.active = true;

	// Projection matrix

	float aspect = (float)vp.width() / (float)vp.height();
	camera.projection = glm::perspective(glm::radians(cam.fov), aspect, cam.near, cam.far);
	// Vulkan needs upside down projection matrix
	camera.projection[1][1] *= -1;

	// View matrix

	float const cos_pitch = std::cos(glm::radians(cam_transform.rotation.x));
	float const cos_yaw = std::cos(glm::radians(cam_transform.rotation.y));
	float const sin_pitch = std::sin(glm::radians(cam_transform.rotation.x));
	float const sin_yaw = std::sin(glm::radians(cam_transform.rotation.y));

	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;

	front.x = cos_pitch * cos_yaw;
	front.y = sin_pitch;
	front.z = cos_pitch * sin_yaw;
	front = glm::normalize(front);
	right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
	up = glm::normalize(glm::cross(right, front));
	camera.view = glm::lookAt(cam_transform.position, cam_transform.position + front, up);

	// Precompute projection-view matrix as its commonly used.
	camera.proj_view = camera.projection * camera.view;

    // Other values
    camera.position = cam_transform.position;
}

void SceneDescription::set_default_albedo(Handle<gfx::Texture> handle) {
    textures.default_albedo = handle;
}

void SceneDescription::set_default_normal(Handle<gfx::Texture> handle) {
    textures.default_normal = handle;
}

void SceneDescription::set_default_metallic(Handle<gfx::Texture> handle) {
    textures.default_metallic = handle;
}

void SceneDescription::set_default_roughness(Handle<gfx::Texture> handle) {
    textures.default_roughness = handle;
}

void SceneDescription::set_default_occlusion(Handle<gfx::Texture> handle) {
    textures.default_occlusion = handle;
}

void SceneDescription::reset() {
	draws.clear();
	textures.views.clear();
	textures.id_to_index.clear();
    point_lights.clear();

	for (auto& cam : cameras) {
		cam.active = false;
	}

    // Push default textures to the texture map
    add_texture(textures.default_albedo);
    add_texture(textures.default_normal);
    add_texture(textures.default_metallic);
    add_texture(textures.default_roughness);
    add_texture(textures.default_occlusion);
}

SceneDescription::MaterialTextures SceneDescription::get_material_textures(Handle<gfx::Material> material) const {
	if (material == Handle<gfx::Material>::none) return {};
	if (!assets::is_ready(material)) return {};

	gfx::Material* mat = assets::get(material);

	return MaterialTextures{
		.albedo = textures.id_to_index[mat->albedo ? mat->albedo : textures.default_albedo],
        .normal = textures.id_to_index[mat->normal ? mat->normal : textures.default_normal],
        .metallic = textures.id_to_index[mat->metallic ? mat->metallic : textures.default_metallic],
        .roughness = textures.id_to_index[mat->roughness ? mat->roughness : textures.default_roughness],
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