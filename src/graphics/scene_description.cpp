#include <andromeda/graphics/scene_description.hpp>
#include <andromeda/graphics/material.hpp>
#include <andromeda/graphics/texture.hpp>
#include <andromeda/assets/assets.hpp>

#include <andromeda/components/environment.hpp>
#include <andromeda/math/transform.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace andromeda::gfx {

bool SceneDescription::is_dirty() const {
    return dirty;
}

void SceneDescription::set_dirty(bool d) {
    dirty = d;
}

void SceneDescription::add_draw(Handle<gfx::Mesh> mesh, Handle<gfx::Material> material, bool occluder, glm::mat4 const& transform) {
    draws.push_back(Draw{mesh, material, occluder});
    draw_transforms.push_back(transform);
}

void SceneDescription::add_material(Handle<gfx::Material> material) {
    // Check if valid handle
    if (material == Handle<gfx::Material>::none) { return; }
    // Check is_ready() before calling get()
    if (!assets::is_ready(material)) { return; }

    gfx::Material* mat = assets::get(material);
    // Add each material texture individually
    add_texture(mat->albedo);
    add_texture(mat->normal);
    add_texture(mat->metal_rough);
    add_texture(mat->occlusion);
}

void SceneDescription::add_light(PointLight const& light, glm::vec3 const& position) {
    gpu::PointLight info{};
    info.pos_radius = glm::vec4(position, light.radius);
    info.color_intensity = glm::vec4(light.color, light.intensity);
    if (light.casts_shadows) info.shadow = 1;
    else info.shadow = -1;
    point_lights.push_back(info);
}

void SceneDescription::add_light(DirectionalLight const& light, glm::vec3 const& rotation) {
    gpu::DirectionalLight info{};
    // We default the shadow index value to -1.
    info.direction_shadow = glm::vec4(glm::normalize(math::euler_to_direction(rotation)), -1.0f);
    // If there is a slot free, we can allocate a shadow index for this lights
    if (light.cast_shadows && num_shadowing_dir_lights < ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS) {
        info.direction_shadow.w = static_cast<float>(num_shadowing_dir_lights);
        num_shadowing_dir_lights += 1;
    }
    info.color_intensity = glm::vec4(light.color, light.intensity);
    directional_lights.push_back(info);
}

void SceneDescription::add_viewport(gfx::Viewport const& vp, thread::LockedValue<const ecs::registry> const& ecs, ecs::entity_t camera) {
    auto const& cam = ecs->get_component<Camera>(camera);
    auto const& transform = ecs->get_component<Transform>(camera);
    CameraInfo& info = cameras[vp.index()];
    info.active = true;

    // If an environment component is present, set the handle accordingly
    if (ecs->has_component<::andromeda::Environment>(camera)) {
        auto const& env = ecs->get_component<::andromeda::Environment>(camera);
        info.environment = env.environment;

        // copy over atmosphere settings since no environment was specified
        if (env.environment == Handle<gfx::Environment>::none || env.environment == get_default_environment()) {
            info.atmosphere.rayleigh = glm::vec4(env.rayleigh, env.rayleigh_height);
            info.atmosphere.mie = glm::vec4(env.mie, env.mie_height);
            info.atmosphere.radii_mie_albedo_g = glm::vec4(env.planet_radius, env.atmosphere_radius, env.mie_albedo, env.mie_g);
            info.atmosphere.ozone_sun = glm::vec4(env.ozone, env.sun_intensity);
        }
    }

    // Postprocessing settings
    if (ecs->has_component<PostProcessingSettings>(camera)) {
        auto const& settings = ecs->get_component<PostProcessingSettings>(camera);
        info.min_log_luminance = settings.min_log_luminance;
        info.max_log_luminance = settings.max_log_luminance;
    }

    // Projection matrix

    float aspect = (float) vp.width() / (float) vp.height();
    info.projection = glm::perspective(glm::radians(cam.fov), aspect, cam.near, cam.far);
    // Vulkan needs upside down projection matrix
    info.projection[1][1] *= -1;

    info.near = cam.near;
    info.far = cam.far;

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
    // Also precompute inverse matrices
    info.inv_projection = glm::inverse(info.projection);
    info.inv_view = glm::inverse(info.view);

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
    dirty = false;
    draws.clear();
    draw_transforms.clear();
    textures.views.clear();
    textures.id_to_index.clear();
    point_lights.clear();
    directional_lights.clear();
    num_shadowing_dir_lights = 0;

    for (auto& cam: cameras) {
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
    if (material == Handle<gfx::Material>::none) { return {}; }
    if (!assets::is_ready(material)) { return {}; }

    gfx::Material* mat = assets::get(material);

    return MaterialTextures{
        .albedo = textures.id_to_index[mat->albedo ? mat->albedo : textures.default_albedo],
        .normal = textures.id_to_index[mat->normal ? mat->normal : textures.default_normal],
        .metal_rough = textures.id_to_index[mat->metal_rough ? mat->metal_rough : textures.default_metal_rough],
        .occlusion = textures.id_to_index[mat->occlusion ? mat->occlusion : textures.default_occlusion]
    };
}


auto SceneDescription::get_draws() const -> std::span<Draw const> {
    return draws;
}

std::span<glm::mat4 const> SceneDescription::get_draw_transforms() const {
    return draw_transforms;
}

auto SceneDescription::get_camera_info(gfx::Viewport const& vp) const -> CameraInfo const& {
    return cameras[vp.index()];
}

Handle<gfx::Texture> SceneDescription::get_brdf_lookup() const {
    return textures.brdf_lut;
}

Handle<gfx::Environment> SceneDescription::get_default_environment() const {
    return default_env;
}

Handle<gfx::Texture> SceneDescription::get_default_albedo() const {
    return textures.default_albedo;
}

std::span<ph::ImageView const> SceneDescription::get_textures() const {
    return textures.views;
}

std::span<gpu::PointLight const> SceneDescription::get_point_lights() const {
    return point_lights;
}

std::span<gpu::DirectionalLight const> SceneDescription::get_directional_lights() const {
    return directional_lights;
}

void SceneDescription::add_texture(Handle<gfx::Texture> texture) {
    // Don't add empty textures, they will be given a default value instead
    if (texture == Handle<gfx::Texture>::none) { return; }
    // Don't add textures that are still loading
    if (!assets::is_ready(texture)) { return; }

    gfx::Texture* tex = assets::get(texture);

    // Add it to the list and set the index mapping
    textures.views.push_back(tex->view);
    textures.id_to_index[texture] = textures.views.size() - 1;
}


}