#ifndef ANDROMEDA_CONTEXT_HPP_
#define ANDROMEDA_CONTEXT_HPP_

#include <phobos/forward.hpp>
#include <memory>

namespace andromeda {

namespace world {
class World;
}

class Context {
public:
	// We need to define the destructor in a cpp file so the unique_ptr deleter works, since it cannot operate on incomplete types
	~Context();
	std::unique_ptr<ph::VulkanContext> vulkan;
	std::unique_ptr<world::World> world;
};

}

#endif