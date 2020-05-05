#ifndef ANDROMEDA_RENDERER_UTIL_HPP_
#define ANDROMEDA_RENDERER_UTIL_HPP_

#include <phobos/renderer/command_buffer.hpp>

namespace andromeda {
namespace renderer {

void auto_viewport_scissor(ph::CommandBuffer& cmd_buf);

}
}

#endif