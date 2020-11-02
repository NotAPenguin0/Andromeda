#ifndef ANDROMEDA_CONTEXT_HPP_
#define ANDROMEDA_CONTEXT_HPP_

#include <phobos/forward.hpp>
#include <memory>

#include <andromeda/ecs/entity.hpp>
#include <andromeda/assets/env_map.hpp>
#include <andromeda/core/task_manager.hpp>
#include <andromeda/util/handle.hpp>
#include <andromeda/util/log.hpp>
#include <andromeda/util/vk_forward.hpp>
#include <andromeda/core/env_map_loader.hpp>

namespace andromeda {

class Texture;
class Mesh;

namespace world {
class World;
}

struct Model { ecs::entity_t id = 0; };

class Context {
public:
	// We need to define the destructor in a cpp file so the unique_ptr deleter works, since it cannot operate on incomplete types
	~Context();
	std::unique_ptr<ph::VulkanContext> vulkan;
	std::unique_ptr<world::World> world;
	std::unique_ptr<TaskManager> tasks;
	std::unique_ptr<EnvMapLoader> envmap_loader;

	Handle<Texture> request_texture(std::string_view path, bool srgb); // TODO: srgb is annoying
	// size is the amount of values in the vertices array
	Handle<Mesh> request_mesh(std::string_view path); // TODO: full model
	Handle<Mesh> request_mesh(float const* vertices, uint32_t size, uint32_t const* indices, uint32_t index_count);
	Handle<Model> request_model(std::string_view path);
	

	Handle<EnvMap> request_env_map(std::string_view path);
};

}

#endif